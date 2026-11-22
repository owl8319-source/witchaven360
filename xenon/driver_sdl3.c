/*
 Witchaven 360: jfaudiolib's SDL output driver (jfaudiolib/src/driver_sdl.c) on SDL3's audio, which SDL-xenon plays
 through xsdk's audio API at 48 kHz. It has the SDL2 driver's names (driver_sdl.h), so jfaudiolib's driver table
 (drivers.c, built with HAVE_SDL) picks it up as it is.

 MultiVoc mixes the game's sounds into a ring of buffers and asks for the next one through MixCallBack; SDL asks for
 audio from its own thread (FeedAudio), which hands over those buffers, and the audio stream converts them from the
 game's rate (22,050 Hz) to the console's. The CD functions say there's no CD.

 Based on driver_sdl.c, Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>, GPL v2 or later.
 */

#include <SDL3/SDL.h>

#include <string.h>

#include "asssys.h"
#include "driver_sdl.h"

enum {
    SDLErr_Warning = -2,
    SDLErr_Error   = -1,
    SDLErr_Ok      = 0,
    SDLErr_Uninitialised,
    SDLErr_InitSubSystem,
    SDLErr_OpenAudio,
    SDLErr_CDNotSupported
};

static int ErrorCode = SDLErr_Ok;
static int Initialised = 0;
static int Playing = 0;

static SDL_AudioStream *Stream = NULL;
static int StreamBytesPerFrame = 4;

static char *MixBuffer = 0;
static int MixBufferSize = 0;
static int MixBufferCount = 0;
static int MixBufferCurrent = 0;
static int MixBufferUsed = 0;
static void ( *MixCallBack )( void ) = 0;

// Copies the next `remaining` bytes of mixed audio into ptr, asking MultiVoc to mix more as each buffer runs out.
static void fillData(Uint8 * ptr, int remaining)
{
    int len;
    char *sptr;

    while (remaining > 0) {
        if (MixBufferUsed == MixBufferSize) {
            MixCallBack();

            MixBufferUsed = 0;
            MixBufferCurrent++;
            if (MixBufferCurrent >= MixBufferCount) {
                MixBufferCurrent -= MixBufferCount;
            }
        }

        while (remaining > 0 && MixBufferUsed < MixBufferSize) {
            sptr = MixBuffer + (MixBufferCurrent * MixBufferSize) + MixBufferUsed;

            len = MixBufferSize - MixBufferUsed;
            if (remaining < len) {
                len = remaining;
            }

            memcpy(ptr, sptr, len);

            ptr += len;
            MixBufferUsed += len;
            remaining -= len;
        }
    }
}

// SDL's audio thread, wanting more audio. The stream is locked while this runs, as SDLDrv_PCM_Lock locks it.
static void SDLCALL FeedAudio(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    Uint8 buffer[4096];

    (void)userdata;
    (void)total_amount;

    if (!Playing) {
        return;
    }

    while (additional_amount > 0) {
        int now = additional_amount < (int)sizeof(buffer) ? additional_amount : (int)sizeof(buffer);

        now -= now % StreamBytesPerFrame;
        if (now <= 0) {
            break;
        }

        fillData(buffer, now);

        if (!SDL_PutAudioStreamData(stream, buffer, now)) {
            return;
        }

        additional_amount -= now;
    }
}


int SDLDrv_GetError(void)
{
    return ErrorCode;
}

const char *SDLDrv_ErrorString( int ErrorNumber )
{
    const char *ErrorString;

    switch( ErrorNumber ) {
        case SDLErr_Warning :
        case SDLErr_Error :
            ErrorString = SDLDrv_ErrorString( ErrorCode );
            break;

        case SDLErr_Ok :
            ErrorString = "SDL Audio ok.";
            break;

        case SDLErr_Uninitialised:
            ErrorString = "SDL Audio uninitialised.";
            break;

        case SDLErr_InitSubSystem:
            ErrorString = "SDL Audio: error in InitSubSystem.";
            break;

        case SDLErr_OpenAudio:
            ErrorString = "SDL Audio: error opening the audio stream.";
            break;

        case SDLErr_CDNotSupported:
            ErrorString = "SDL CD is not supported";
            break;

        default:
            ErrorString = "Unknown SDL Audio error code.";
            break;
    }

    return ErrorString;
}

int SDLDrv_PCM_Init(int * mixrate, int * numchannels, int * samplebits, void * initdata)
{
    SDL_AudioSpec spec;

    (void)initdata;

    if (Initialised) {
        SDLDrv_PCM_Shutdown();
    }

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        ASS_Message("SDLDrv: SDL_InitSubSystem failed: %s\n", SDL_GetError());
        ErrorCode = SDLErr_InitSubSystem;
        return SDLErr_Error;
    }

    if (*numchannels != 1 && *numchannels != 2) {
        *numchannels = 2;
    }
    if (*samplebits != 8 && *samplebits != 16) {
        *samplebits = 16;
    }

    // SDL_AUDIO_S16 is the machine's own byte order, as MultiVoc mixes it.
    memset(&spec, 0, sizeof(spec));
    spec.format = (*samplebits == 8) ? SDL_AUDIO_U8 : SDL_AUDIO_S16;
    spec.channels = *numchannels;
    spec.freq = *mixrate;
    StreamBytesPerFrame = (*samplebits / 8) * *numchannels;

    Stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedAudio, NULL);
    if (Stream == NULL) {
        ASS_Message("SDLDrv: SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        ErrorCode = SDLErr_OpenAudio;
        return SDLErr_Error;
    }

    ASS_Message("SDLDrv: %d Hz, %d-bit, %d channel(s), through SDL %s\n", *mixrate, *samplebits, *numchannels,
                SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "(unknown)");

    Initialised = 1;
    return SDLErr_Ok;
}

void SDLDrv_PCM_Shutdown(void)
{
    if (!Initialised) {
        return;
    }

    SDLDrv_PCM_StopPlayback();

    SDL_DestroyAudioStream(Stream);
    Stream = NULL;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    Initialised = 0;
}

int SDLDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
                        int NumDivisions, void ( *CallBackFunc )( void ) )
{
    if (!Initialised) {
        ErrorCode = SDLErr_Uninitialised;
        return SDLErr_Error;
    }

    if (Playing) {
        SDLDrv_PCM_StopPlayback();
    }

    SDL_LockAudioStream(Stream);

    MixBuffer = BufferStart;
    MixBufferSize = BufferSize;
    MixBufferCount = NumDivisions;
    MixBufferCurrent = 0;
    MixBufferUsed = 0;
    MixCallBack = CallBackFunc;

    // prime the buffer
    MixCallBack();

    Playing = 1;

    SDL_UnlockAudioStream(Stream);

    if (!SDL_ResumeAudioStreamDevice(Stream)) {
        ASS_Message("SDLDrv: SDL_ResumeAudioStreamDevice failed: %s\n", SDL_GetError());
    }

    return SDLErr_Ok;
}

void SDLDrv_PCM_StopPlayback(void)
{
    if (!Initialised || !Playing) {
        return;
    }

    SDL_PauseAudioStreamDevice(Stream);

    SDL_LockAudioStream(Stream);
    Playing = 0;
    SDL_UnlockAudioStream(Stream);
}

void SDLDrv_PCM_Lock(void)
{
    if (Stream) SDL_LockAudioStream(Stream);
}

void SDLDrv_PCM_Unlock(void)
{
    if (Stream) SDL_UnlockAudioStream(Stream);
}


int SDLDrv_CD_Init(void)
{
    ErrorCode = SDLErr_CDNotSupported;
    return SDLErr_Error;
}

void SDLDrv_CD_Shutdown(void)
{
}

int SDLDrv_CD_Play(int track, int loop)
{
    (void)track; (void)loop;
    ErrorCode = SDLErr_CDNotSupported;
    return SDLErr_Error;
}

void SDLDrv_CD_Stop(void)
{
}

void SDLDrv_CD_Pause(int pauseon)
{
    (void)pauseon;
}

int SDLDrv_CD_IsPlaying(void)
{
    return 0;
}

void SDLDrv_CD_SetVolume(int volume)
{
    (void)volume;
}
