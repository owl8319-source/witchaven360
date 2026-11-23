// Witchaven 360: the music, as Witchaven played it on a Sound Blaster 16: its songs' FM arrangements on an OPL3 chip,
// with the game's own instrument banks, melodic.bnk and drum.bnk. The chip is Nuked OPL3 (opl/opl3.c), emulated on
// SDL3's audio thread in an audio stream of its own, beside the sound effects' (driver_sdl3.c).
//
// jfaudiolib reads the song (midi.c) and calls this driver's functions for each MIDI event, from service(), once a MIDI
// tick (tempo x division / 60 times a second, as SetTempo says). Here service() is called from the audio thread, at
// the right sample between the chip's output, so the timing is exact to the sample, and the events reach the chip
// with no locking of their own. Lock and Unlock lock that audio stream, for midi.c's calls from the game.
//
// The banks are AdLib .BNK files: 128 instruments, each two operators. melodic.bnk is indexed by the MIDI program;
// drum.bnk by the note on MIDI channel 10, each drum played at the fixed note its name record gives (the byte HMI's
// driver kept there: 35 for the kick, 60 for the snare). The OPL3's 18 two-operator voices go to the notes as they
// come, the longest-released voice first, the oldest note's when all are sounding.

#include <SDL3/SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opl3.h"

#include "asssys.h"
#include "driver_opl.h"

#define MUSIC_RATE 48000
#define VOICES 18
#define CHANNELS 16
#define DRUM_CHANNEL 9
#define BANK_INSTRUMENTS 128
#define BANK_HEADER_BYTES 28
#define BANK_RECORD_BYTES 30
// The OPL's own clock: what an F-number is counted in.
#define OPL_CLOCK_HZ 49716.0
// The music's overall level against the sound effects, of 256: the OPL's output, 18 voices' worth of headroom, is
// quiet beside the game's 8-bit sounds.
#define MUSIC_GAIN 512

typedef struct {
    unsigned char ksl, mult, fb, ar, sl, eg, dr, rr, tl, am, vib, ksr, con;
} oploperator_t;

typedef struct {
    oploperator_t mod, car;
    unsigned char modwave, carwave;
    unsigned char fixednote;    // drum.bnk: the note the drum plays at
} oplinstrument_t;

typedef struct {
    int on;                     // keyed on
    int sustained;              // let go while the sustain pedal was down; keyed off when it comes up
    int channel, note;
    unsigned age;               // when it was keyed on, or off
    const oplinstrument_t *ins; // what's programmed into it
    int playnote;               // the note sounded (a drum's fixed note)
    int velocity;
} oplvoice_t;

typedef struct {
    int program, volume, expression, pan, sustain, bend;  // bend: -8192 to 8191
} oplchannel_t;

static const unsigned char op_offsets[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };

static oplinstrument_t melodic[BANK_INSTRUMENTS];
static oplinstrument_t drums[BANK_INSTRUMENTS];
static oplvoice_t voices[VOICES];
static oplchannel_t channels[CHANNELS];
static unsigned clock_age;

static opl3_chip chip;
static SDL_AudioStream *stream = NULL;
static int master_volume = 255;
static void (*service)(void) = NULL;
static double samples_per_tick = 0.0;
static double samples_to_tick = 0.0;

// ---------------------------------------------------------------------------------------------------------------------
// The banks

static void ReadOperator(oploperator_t *op, const unsigned char *b)
{
    op->ksl = b[0]; op->mult = b[1]; op->fb = b[2]; op->ar = b[3]; op->sl = b[4]; op->eg = b[5]; op->dr = b[6];
    op->rr = b[7]; op->tl = b[8]; op->am = b[9]; op->vib = b[10]; op->ksr = b[11]; op->con = b[12];
}

static int LoadBank(const char *name, oplinstrument_t *bank)
{
    FILE *f = fopen(name, "rb");
    unsigned char *data;
    long size;
    int i, count, names, records;

    if (!f) {
        ASS_Message("OPLDrv: no %s\n", name);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (unsigned char *)malloc(size > 0 ? size : 1);
    if (!data || fread(data, 1, size, f) != (size_t)size || size < BANK_HEADER_BYTES || memcmp(data + 2, "ADLIB-", 6)) {
        ASS_Message("OPLDrv: %s isn't an AdLib bank\n", name);
        free(data);
        fclose(f);
        return -1;
    }
    fclose(f);

    // Little-endian: the instrument count, then where the name records and the instrument data start.
    count = data[10] | (data[11] << 8);
    names = data[12] | (data[13] << 8) | (data[14] << 16) | (data[15] << 24);
    records = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);
    if (count > BANK_INSTRUMENTS) count = BANK_INSTRUMENTS;

    for (i = 0; i < count; i++) {
        const unsigned char *n = data + names + i * 12;
        const unsigned char *r = data + records + i * BANK_RECORD_BYTES;
        int index;

        if (names + (i + 1) * 12 > size || records + (i + 1) * BANK_RECORD_BYTES > size) break;

        index = n[0] | (n[1] << 8);
        if (index >= BANK_INSTRUMENTS) continue;

        // Each record: the percussive mode and voice (unused), the two operators, their waveforms.
        ReadOperator(&bank[index].mod, r + 2);
        ReadOperator(&bank[index].car, r + 15);
        bank[index].modwave = r[28];
        bank[index].carwave = r[29];
        bank[index].fixednote = n[2] & 0x7f;
    }

    free(data);
    ASS_Message("OPLDrv: %s, %d instruments\n", name, count);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// The chip

static void Write(int voice, int reg, int value)
{
    OPL3_WriteReg(&chip, (uint16_t)((voice >= 9 ? 0x100 : 0) + reg), (uint8_t)value);
}

static void WriteOperator(int voice, int offset, const oploperator_t *op, int tl, int wave)
{
    const int o = op_offsets[voice % 9] + offset;

    Write(voice, 0x20 + o, (op->am << 7) | (op->vib << 6) | (op->eg << 5) | (op->ksr << 4) | (op->mult & 15));
    Write(voice, 0x40 + o, ((op->ksl & 3) << 6) | (tl & 63));
    Write(voice, 0x60 + o, ((op->ar & 15) << 4) | (op->dr & 15));
    Write(voice, 0x80 + o, ((op->sl & 15) << 4) | (op->rr & 15));
    Write(voice, 0xE0 + o, wave & 7);
}

// How much quieter than the instrument as written, in the OPL's 0.75 dB steps: velocity, the channel's volume and
// expression, taken as amplitudes (20 log10 of the fraction, all three together). General MIDI's 40 log10 left the
// FM arrangements' quiet passages (velocity 48 to 64) too quiet beside the sound effects.
static int Attenuation(const oplvoice_t *v)
{
    const oplchannel_t *ch = &channels[v->channel];
    double level = (v->velocity / 127.0) * (ch->volume / 127.0) * (ch->expression / 127.0);
    double db;

    if (level <= 0.0) return 63;
    db = -20.0 * log10(level);
    if (db > 47.0) return 63;
    return (int)(db / 0.75 + 0.5);
}

static void WriteLevels(int i)
{
    const oplvoice_t *v = &voices[i];
    const oplinstrument_t *ins = v->ins;
    const int att = Attenuation(v);
    int car = ins->car.tl + att, mod = ins->mod.tl;
    const int c = op_offsets[i % 9];

    if (car > 63) car = 63;
    // In additive mode, the modulator is heard too, and is made quieter with the carrier.
    if (ins->mod.con & 1) {
        mod += att;
        if (mod > 63) mod = 63;
    }

    Write(i, 0x40 + c, ((ins->mod.ksl & 3) << 6) | mod);
    Write(i, 0x40 + c + 3, ((ins->car.ksl & 3) << 6) | car);
}

static void WriteConnection(int i)
{
    const oplvoice_t *v = &voices[i];
    const int pan = channels[v->channel].pan;
    // OPL3 stereo: left 0x10, right 0x20; hard left or right only when the song pans well over.
    const int sides = pan < 32 ? 0x10 : pan > 96 ? 0x20 : 0x30;

    Write(i, 0xC0 + i % 9, sides | ((v->ins->mod.fb & 7) << 1) | (v->ins->mod.con & 1));
}

static void WriteFrequency(int i, int keyon)
{
    const oplvoice_t *v = &voices[i];
    const double semitones = (v->playnote - 69) + (v->channel == DRUM_CHANNEL ? 0.0 : channels[v->channel].bend * 2.0 / 8192.0);
    const double hz = 440.0 * pow(2.0, semitones / 12.0);
    int block, fnum = 0;

    for (block = 0; block < 8; block++) {
        fnum = (int)(hz * (double)(1 << (20 - block)) / OPL_CLOCK_HZ + 0.5);
        if (fnum < 1024) break;
    }
    if (block > 7) { block = 7; fnum = 1023; }

    Write(i, 0xA0 + i % 9, fnum & 0xff);
    Write(i, 0xB0 + i % 9, (keyon ? 0x20 : 0) | (block << 2) | ((fnum >> 8) & 3));
}

static void KeyOff(int i)
{
    voices[i].on = 0;
    voices[i].sustained = 0;
    voices[i].age = ++clock_age;
    WriteFrequency(i, 0);
}

// A voice for a new note: the one let go longest ago, or, all sounding, the one keyed on longest ago.
static int FindVoice(void)
{
    int i, best = -1;

    for (i = 0; i < VOICES; i++) {
        if (!voices[i].on && (best < 0 || voices[i].age < voices[best].age)) best = i;
    }
    if (best >= 0) return best;

    for (i = 0; i < VOICES; i++) {
        if (best < 0 || voices[i].age < voices[best].age) best = i;
    }
    KeyOff(best);
    return best;
}

static void ResetChip(void)
{
    int i;

    OPL3_Reset(&chip, MUSIC_RATE);
    OPL3_WriteReg(&chip, 0x105, 0x01);  // OPL3 mode: 18 voices, stereo, waveforms 4 to 7
    OPL3_WriteReg(&chip, 0x104, 0x00);  // two operators each
    OPL3_WriteReg(&chip, 0x001, 0x20);
    OPL3_WriteReg(&chip, 0x0BD, 0x00);  // melodic mode, no OPL rhythm section

    memset(voices, 0, sizeof(voices));
    for (i = 0; i < VOICES; i++) voices[i].ins = &melodic[0];
    clock_age = 0;
}

static void ResetChannels(void)
{
    int c;

    for (c = 0; c < CHANNELS; c++) {
        channels[c].program = 0;
        channels[c].volume = 100;
        channels[c].expression = 127;
        channels[c].pan = 64;
        channels[c].sustain = 0;
        channels[c].bend = 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// The MIDI functions (midifuncs), on the audio thread or with the stream locked

static void Func_NoteOff(int channel, int key, int velocity)
{
    int i;

    (void)velocity;

    for (i = 0; i < VOICES; i++) {
        if (voices[i].on && voices[i].channel == channel && voices[i].note == key && !voices[i].sustained) {
            if (channels[channel].sustain) voices[i].sustained = 1;
            else KeyOff(i);
        }
    }
}

static void Func_NoteOn(int channel, int key, int velocity)
{
    const oplinstrument_t *ins;
    int i, playnote;

    if (velocity == 0) {
        Func_NoteOff(channel, key, 0);
        return;
    }

    if (channel == DRUM_CHANNEL) {
        ins = &drums[key & 127];
        playnote = ins->fixednote;
    } else {
        ins = &melodic[channels[channel].program & 127];
        playnote = key;
    }

    // The same note again on the channel takes the voice it had.
    for (i = 0; i < VOICES; i++) {
        if (voices[i].on && voices[i].channel == channel && voices[i].note == key) break;
    }
    if (i == VOICES) i = FindVoice();
    else WriteFrequency(i, 0);

    voices[i].on = 1;
    voices[i].sustained = 0;
    voices[i].channel = channel;
    voices[i].note = key;
    voices[i].playnote = playnote;
    voices[i].velocity = velocity;
    voices[i].age = ++clock_age;
    voices[i].ins = ins;

    WriteOperator(i, 0, &ins->mod, ins->mod.tl, ins->modwave);
    WriteOperator(i, 3, &ins->car, ins->car.tl, ins->carwave);
    WriteLevels(i);
    WriteConnection(i);
    WriteFrequency(i, 1);
}

static void Func_PolyAftertouch(int channel, int key, int pressure)
{
    (void)channel; (void)key; (void)pressure;
}

static void AllNotesOff(int channel)
{
    int i;

    for (i = 0; i < VOICES; i++) {
        if (voices[i].on && (channel < 0 || voices[i].channel == channel)) KeyOff(i);
    }
}

static void Func_ControlChange(int channel, int number, int value)
{
    oplchannel_t *ch = &channels[channel];
    int i;

    switch (number) {
        case 7:     // volume
        case 11:    // expression
            if (number == 7) ch->volume = value; else ch->expression = value;
            for (i = 0; i < VOICES; i++) {
                if (voices[i].on && voices[i].channel == channel) WriteLevels(i);
            }
            break;
        case 10:    // pan
            ch->pan = value;
            for (i = 0; i < VOICES; i++) {
                if (voices[i].on && voices[i].channel == channel) WriteConnection(i);
            }
            break;
        case 64:    // sustain pedal
            ch->sustain = value >= 64;
            if (!ch->sustain) {
                for (i = 0; i < VOICES; i++) {
                    if (voices[i].on && voices[i].channel == channel && voices[i].sustained) KeyOff(i);
                }
            }
            break;
        case 121:   // reset controllers
            ch->volume = 100; ch->expression = 127; ch->pan = 64; ch->bend = 0;
            ch->sustain = 0;
            break;
        case 120:   // all sound off
        case 123:   // all notes off
            AllNotesOff(channel);
            break;
        default:
            break;
    }
}

static void Func_ProgramChange(int channel, int program)
{
    channels[channel].program = program & 127;
}

static void Func_ChannelAftertouch(int channel, int pressure)
{
    (void)channel; (void)pressure;
}

static void Func_PitchBend(int channel, int lsb, int msb)
{
    int i;

    channels[channel].bend = ((msb << 7) | lsb) - 8192;
    for (i = 0; i < VOICES; i++) {
        if (voices[i].on && voices[i].channel == channel) WriteFrequency(i, 1);
    }
}

static void Func_ReleasePatches(void)
{
}

static void Func_LoadPatch(int number)
{
    (void)number;
}

static void Func_SetVolume(int volume)
{
    master_volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
}

static int Func_GetVolume(void)
{
    return master_volume;
}

// ---------------------------------------------------------------------------------------------------------------------
// The audio stream

// SDL's audio thread, wanting more music: the chip's output, sample by sample, with the song's ticks between.
static void SDLCALL FeedMusic(void *userdata, SDL_AudioStream *s, int additional_amount, int total_amount)
{
    int16_t buffer[512 * 2];
    int frames = additional_amount / (int)(2 * sizeof(int16_t));

    (void)userdata;
    (void)total_amount;

    while (frames > 0) {
        int now = frames < 512 ? frames : 512;
        int done = 0, i;

        while (done < now) {
            int run = now - done;

            if (service && samples_per_tick > 0.0) {
                if (samples_to_tick < 1.0) {
                    service();
                    samples_to_tick += samples_per_tick;
                    continue;
                }
                if (run > (int)samples_to_tick) run = (int)samples_to_tick;
            }

            OPL3_GenerateStream(&chip, buffer + done * 2, (uint32_t)run);
            done += run;
            if (service && samples_per_tick > 0.0) samples_to_tick -= run;
        }

        for (i = 0; i < now * 2; i++) {
            int v = buffer[i] * master_volume / 255 * MUSIC_GAIN / 256;
            buffer[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        }

        if (!SDL_PutAudioStreamData(s, buffer, now * (int)(2 * sizeof(int16_t)))) {
            return;
        }
        frames -= now;
    }
}

int OPLDrv_MIDI_Init(midifuncs *funcs, const char *params)
{
    const SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, MUSIC_RATE };

    (void)params;

    OPLDrv_MIDI_Shutdown();
    memset(funcs, 0, sizeof(midifuncs));

    memset(melodic, 0, sizeof(melodic));
    memset(drums, 0, sizeof(drums));
    if (LoadBank("melodic.bnk", melodic) < 0 || LoadBank("drum.bnk", drums) < 0) {
        return -1;
    }

    ResetChip();
    ResetChannels();

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        ASS_Message("OPLDrv: SDL_InitSubSystem failed: %s\n", SDL_GetError());
        return -1;
    }

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedMusic, NULL);
    if (stream == NULL) {
        ASS_Message("OPLDrv: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return -1;
    }

    if (!SDL_ResumeAudioStreamDevice(stream)) {
        ASS_Message("OPLDrv: SDL_ResumeAudioStreamDevice failed: %s\n", SDL_GetError());
    }

    funcs->NoteOff = Func_NoteOff;
    funcs->NoteOn = Func_NoteOn;
    funcs->PolyAftertouch = Func_PolyAftertouch;
    funcs->ControlChange = Func_ControlChange;
    funcs->ProgramChange = Func_ProgramChange;
    funcs->ChannelAftertouch = Func_ChannelAftertouch;
    funcs->PitchBend = Func_PitchBend;
    funcs->ReleasePatches = Func_ReleasePatches;
    funcs->LoadPatch = Func_LoadPatch;
    funcs->SetVolume = Func_SetVolume;
    funcs->GetVolume = Func_GetVolume;

    ASS_Message("OPLDrv: OPL3 music at %d Hz\n", MUSIC_RATE);
    return 0;
}

void OPLDrv_MIDI_Shutdown(void)
{
    if (stream == NULL) {
        return;
    }

    OPLDrv_MIDI_HaltPlayback();
    SDL_DestroyAudioStream(stream);
    stream = NULL;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int OPLDrv_MIDI_StartPlayback(void (*func)(void))
{
    OPLDrv_MIDI_Lock();
    service = func;
    samples_to_tick = 0.0;
    OPLDrv_MIDI_Unlock();
    return 0;
}

void OPLDrv_MIDI_HaltPlayback(void)
{
    OPLDrv_MIDI_Lock();
    service = NULL;
    AllNotesOff(-1);
    ResetChannels();
    OPLDrv_MIDI_Unlock();
}

void OPLDrv_MIDI_SetTempo(int tempo, int division)
{
    const double ticks_per_second = (double)tempo * (double)division / 60.0;

    OPLDrv_MIDI_Lock();
    samples_per_tick = ticks_per_second > 0.0 ? MUSIC_RATE / ticks_per_second : 0.0;
    OPLDrv_MIDI_Unlock();
}

void OPLDrv_MIDI_Lock(void)
{
    if (stream) SDL_LockAudioStream(stream);
}

void OPLDrv_MIDI_Unlock(void)
{
    if (stream) SDL_UnlockAudioStream(stream);
}
