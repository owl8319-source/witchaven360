// Witchaven 360: the Build engine's platform layer for the Xbox 360, through xsdk and SDL-xenon. It takes the place of
// jfbuild's SDL2 layer (jfbuild/src/sdlayer2.c) and provides the same functions (jfbuild/include/baselayer.h).
//
// Video: the engine draws each frame into `frame`, xres x yres bytes of palette indexes. showframe() turns them into
// colours through the engine's current palette, uploads them into a texture and has the GPU draw it, point sampled,
// on a quad in the middle of the 1280x720 display, as Heretic 360 does (Heretic 360's xenon/i_video.c). 320x200 and
// 640x400 are shown 4:3, as on a PC's monitor; other modes keep square pixels, the engine correcting the aspect itself.
//
// Input: the controller is read here, through SDL3, and handed to xenon/wh_pad.c (xenon_pad_update), which turns it
// into the keys and stick movement the game reads. There's no keyboard or mouse.
//
// Timer, log and startup are as Heretic 360 has them: SDL3's timer, stdout into xsdk's log beside the .xex, written
// out once a second by a thread of its own, so the game never waits on the hard drive to log a line.

#include "build.h"

#include <SDL3/SDL.h>

#include <stdlib.h>
#include <string.h>

#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/texture.h"
#include "xsdk/xam.h"

#include "witchaven_shaders.h"

#include "baselayer.h"
#include "baselayer_priv.h"
#include "cache1d.h"
#include "pragmas.h"
#include "a.h"
#include "osd.h"

#include "xenonlayer.h"

int displaycnt = 1;

static char apptitle[256] = "Witchaven";

// ---------------------------------------------------------------------------------------------------------------------
// The log

#define LOG_BUFFER_BYTES (64 * 1024)
#define LOG_WRITE_INTERVAL_MS 1000

static char log_buffer[LOG_BUFFER_BYTES];
static char error_log_buffer[LOG_BUFFER_BYTES / 4];

static int SDLCALL LogWriter(void *data)
{
    (void)data;

    for (;;) {
        SDL_Delay(LOG_WRITE_INTERVAL_MS);
        fflush(stderr);
        fflush(stdout);
    }

    return 0;
}

static void StartLogWriter(void)
{
    SDL_Thread *thread;

    setvbuf(stdout, log_buffer, _IOFBF, sizeof(log_buffer));
    setvbuf(stderr, error_log_buffer, _IOFBF, sizeof(error_log_buffer));

    thread = SDL_CreateThread(LogWriter, "witchaven360 log", NULL);
    if (thread == NULL) {
        // Without the writer, lines go to the log as they're finished.
        setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
        setvbuf(stderr, NULL, _IONBF, 0);
        printf("witchaven360: no log writer thread (%s), so lines are logged as they come\n", SDL_GetError());
        return;
    }

    SDL_DetachThread(thread);
}

// Writes out the log at once: before a crash message, and as the game exits.
static void FlushLog(void)
{
    fflush(stderr);
    fflush(stdout);
}

// ---------------------------------------------------------------------------------------------------------------------
// Window-manager functions: there are no windows. Messages go to the log.

int wm_msgbox(const char *name, const char *fmt, ...)
{
    va_list va;

    printf("witchaven360: [%s] ", name ? name : apptitle);
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf("\n");
    FlushLog();

    return 1;
}

int wm_ynbox(const char *name, const char *fmt, ...)
{
    va_list va;

    printf("witchaven360: [%s] ", name ? name : apptitle);
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf(" (answered no)\n");
    FlushLog();

    return 0;
}

int wm_filechooser(const char *initialdir, const char *initialfile, const char *type, int foropen, char **choice)
{
    (void)initialdir; (void)initialfile; (void)type; (void)foropen; (void)choice;
    return -1;
}

int wm_idle(void *ptr)
{
    (void)ptr;
    return 0;
}

void wm_setapptitle(const char *name)
{
    if (name) {
        strncpy(apptitle, name, sizeof(apptitle) - 1);
        apptitle[sizeof(apptitle) - 1] = 0;
    }
}

void wm_setwindowtitle(const char *name)
{
    (void)name;
}

void wm_allowbackgroundidle(int onf)
{
    (void)onf;
}

void wm_allowtaskswitching(int onf)
{
    (void)onf;
}

// ---------------------------------------------------------------------------------------------------------------------
// System

int main(int argc, char *argv[])
{
    int r;

    // The loader passes the .xex's name alone. xsdk sets the current directory to game:\, the folder the .xex was
    // launched from, where the game's data is.
    StartLogWriter();
    printf("witchaven360: starting Witchaven 360 " WITCHAVEN360_VERSION "\n");

    _buildargc = argc;
    _buildargv = (const char **)argv;

    startwin_open();
    baselayer_init();

    r = app_main(_buildargc, (char const * const *)_buildargv);

    startwin_close();
    printf("witchaven360: app_main returned %d\n", r);
    FlushLog();

    SDL_Quit();

    return r;
}

int initsystem(void)
{
    printf("witchaven360: SDL %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    atexit(uninitsystem);
    return 0;
}

void uninitsystem(void)
{
    uninitinput();
    uninittimer();
    FlushLog();
}

void initputs(const char *str)
{
    (void)str;
}

void debugprintf(const char *f, ...)
{
#ifdef DEBUGGINGAIDS
    va_list va;

    va_start(va, f);
    vprintf(f, va);
    va_end(va);
#endif
    (void)f;
}

// ---------------------------------------------------------------------------------------------------------------------
// Input: the controller, and keys made up from it (xenon/wh_pad.c)

static SDL_Gamepad *gamepad = NULL;
static int gamepad_initialized = 0;
static int system_ui_was_open = 0;

static void OpenFirstGamepad(void)
{
    SDL_JoystickID *ids;
    int count = 0;

    ids = SDL_GetGamepads(&count);
    if (ids == NULL) {
        return;
    }

    if (count > 0) {
        gamepad = SDL_OpenGamepad(ids[0]);
        if (gamepad == NULL) {
            printf("witchaven360: SDL_OpenGamepad failed: %s\n", SDL_GetError());
        } else {
            printf("witchaven360: using controller \"%s\"\n", SDL_GetGamepadName(gamepad));
        }
    }

    SDL_free(ids);
}

int initinput(void)
{
    inputdevices = 1;   // "keyboard": the keys wh_pad.c makes up
    joynumaxes = 4;     // the sticks: left x and y, right x and y, as the game reads them (whinp.c)
    joynumbuttons = 0;  // the buttons become keys instead

    if (gamepad_initialized) {
        return 0;
    }

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
        printf("witchaven360: SDL_InitSubSystem(SDL_INIT_GAMEPAD) failed: %s\n", SDL_GetError());
        return 0;
    }

    gamepad_initialized = 1;
    inputdevices |= 4;
    OpenFirstGamepad();

    return 0;
}

void uninitinput(void)
{
    if (gamepad != NULL) {
        SDL_CloseGamepad(gamepad);
        gamepad = NULL;
    }

    if (gamepad_initialized) {
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        gamepad_initialized = 0;
    }
}

const char *getkeyname(int num)
{
    static char name[8];

    if ((unsigned)num >= 256) return NULL;
    snprintf(name, sizeof(name), "0x%02X", num);
    return name;
}

const char *getjoyname(int what, int num)
{
    static const char *const axes[] = { "Left X", "Left Y", "Right X", "Right Y" };

    if (what == 0 && num >= 0 && num < 4) return axes[num];
    return NULL;
}

int initmouse(void) { return 0; }
void uninitmouse(void) { }
void grabmouse(int a) { (void)a; mousex = mousey = 0; }
void readmousexy(int *x, int *y) { *x = *y = 0; }
void readmousebstatus(int *b) { *b = 0; }

void releaseallbuttons(void)
{
    xenon_releaseallkeys();
}

// A key going down or up, as SDL2's layer posts a keyboard key (sdlayer2.c's handleevents).
void xenon_keyevent(int code, int down)
{
    if (code <= 0 || code > 255) {
        return;
    }

    if (down) {
        if (!keystatus[code]) keystatus[code] = 1;
    } else {
        keystatus[code] = 0;
    }

    keyfifo[keyfifoend] = code;
    keyfifo[(keyfifoend + 1) & (KEYFIFOSIZ - 1)] = down ? 1 : 0;
    keyfifoend = ((keyfifoend + 2) & (KEYFIFOSIZ - 1));
}

// A character typed, for the game's text entry (bgetchar).
void xenon_charevent(int ch)
{
    if (((keyasciififoend + 1) & (KEYFIFOSIZ - 1)) != keyasciififoplc) {
        keyasciififo[keyasciififoend] = (unsigned char)ch;
        keyasciififoend = ((keyasciififoend + 1) & (KEYFIFOSIZ - 1));
    }
}

int xenon_pad_connected(void)
{
    return gamepad != NULL;
}

int xenon_pad_button(int button)
{
    if (gamepad == NULL) return 0;
    return SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)button) ? 1 : 0;
}

int xenon_pad_axis(int axis)
{
    if (gamepad == NULL) return 0;
    return SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)axis);
}

void xenon_pad_rumble(int low, int high, int ms)
{
    if (gamepad == NULL) return;
    SDL_RumbleGamepad(gamepad, (Uint16)low, (Uint16)high, (Uint32)ms);
}

int xenon_system_ui_open(void)
{
    return system_ui_was_open;
}

static void ReadController(void)
{
    // Nothing else reads SDL's events, so they're dropped rather than left to pile up.
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    // Asked once a frame, controller or not (xsdk's docs/GUIDE.md).
    system_ui_was_open = xsdk_system_ui_open() ? 1 : 0;

    if (!gamepad_initialized) {
        return;
    }

    if (gamepad != NULL && !SDL_GamepadConnected(gamepad)) {
        printf("witchaven360: controller disconnected\n");
        SDL_CloseGamepad(gamepad);
        gamepad = NULL;
    }

    if (gamepad == NULL) {
        static unsigned int last_try = 0;
        const unsigned int now = getticks();

        // Looks for a controller twice a second, not every frame.
        if (now - last_try >= 500) {
            last_try = now;
            OpenFirstGamepad();
        }
    }
}

int handleevents(void)
{
    ReadController();
    xenon_pad_update();

    sampletimer();
    startwin_idle(NULL);

    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Timer

static Uint64 timerstartns = 0;
static Uint64 timerlastsample = 0;
static int timerticspersec = 0;
static void (*usertimercallback)(void) = NULL;

static Uint64 TimerTics(void)
{
    return (SDL_GetTicksNS() - timerstartns) * (Uint64)timerticspersec / 1000000000ull;
}

int inittimer(int tickspersecond, void (*callback)(void))
{
    if (timerticspersec) return 0;   // already installed

    buildputs("Initialising timer\n");

    timerstartns = SDL_GetTicksNS();
    timerticspersec = tickspersecond;
    timerlastsample = 0;
    usertimercallback = callback;

    return 0;
}

void uninittimer(void)
{
    timerticspersec = 0;
}

void sampletimer(void)
{
    Uint64 now;
    int n;

    if (!timerticspersec) return;

    now = TimerTics();
    n = (int)(now - timerlastsample);
    if (n > 0) {
        totalclock += n;
        timerlastsample += n;
    }

    if (usertimercallback) for (; n > 0; n--) usertimercallback();
}

unsigned int getticks(void)
{
    return (unsigned int)SDL_GetTicks();
}

unsigned int getusecticks(void)
{
    return (unsigned int)(SDL_GetTicksNS() / 1000);
}

int gettimerfreq(void)
{
    return timerticspersec;
}

// ---------------------------------------------------------------------------------------------------------------------
// Video

enum {
    BUFFERS = 2,
    SWAP_INTERVAL = 1,
    // The Guide's first opening held a frame up 2.7 s (xsdk docs/HARDWARE.md, "The Guide, milestone 37").
    BEGIN_TIMEOUT_MS = 5000,
    FENCE_TIMEOUT_MS = 1000,
    CLEAR_ARGB = 0xFF000000,
    // Two triangles: position then texture coordinates, blit.vert's a_position and a_uv.
    QUAD_VERTICES = 6,
    VERTEX_FLOATS = 4,
    STRIDE_BYTES = VERTEX_FLOATS * 4,
    UV_OFFSET_BYTES = 8,
    QUAD_BYTES = QUAD_VERTICES * STRIDE_BYTES,
    // Each xsdk_gpu_alloc takes at least 64 KiB (xsdk docs/PITFALLS.md).
    VERTEX_ALLOC_BYTES = 64 * 1024,
    FRAME_TEXTURES = 2,
    // The largest frame the game may ask for: the display's own size.
    MAX_FRAME_WIDTH = 1280,
    MAX_FRAME_HEIGHT = 720,
};

static const XsdkVertexElement quad_layout[] = {
    { .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = 0 },
    { .usage = XSDK_USAGE_TEXCOORD, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = UV_OFFSET_BYTES },
};

static int gpu_open = 0;
static int gpu_failed = 0;
static XsdkShader vertex_shader;
static XsdkShader pixel_shader;
static volatile uint32_t *quad_vertices;
static XsdkTexture frame_textures[FRAME_TEXTURES];
static uint32_t frame_fences[FRAME_TEXTURES];
static XsdkSamplerState frame_sampler;
static uint32_t display_width;
static uint32_t display_height;
static unsigned int frame_count;
static int texture_width, texture_height;

// The engine's frame, palette indexes, and the same as colours (0xAARRGGBB) before it's uploaded.
static unsigned char *frame = NULL;
static uint32_t *argb_frame = NULL;
static uint32_t palette_argb[256];

// Writes words into GPU memory with volatile 32-bit stores, then drops the GPU's cached copy.
static void WriteGPU(volatile uint32_t *memory, const void *data, uint32_t size_bytes)
{
    uint32_t i, word;

    for (i = 0; i < size_bytes / 4; i++) {
        memcpy(&word, (const uint8_t *)data + i * 4, 4);
        memory[i] = word;
    }

    xsdk_gpu_invalidate(xsdk_gpu_address(memory), size_bytes);
}

static int OpenGPU(void)
{
    NTSTATUS status;
    const XsdkSampler sampler = {
        .mag_filter = XSDK_FILTER_POINT,
        .min_filter = XSDK_FILTER_POINT,
        .mip_filter = XSDK_FILTER_POINT,
        .address_u = XSDK_ADDRESS_CLAMP,
        .address_v = XSDK_ADDRESS_CLAMP,
    };

    if (gpu_open) {
        return 0;
    }

    status = xsdk_gpu_open();
    if (!NT_SUCCESS(status)) {
        printf("witchaven360: xsdk_gpu_open failed: 0x%08X\n", (unsigned)status);
        return -1;
    }

    status = xsdk_shader_load(&vertex_shader, blit_vert.container, blit_vert.container_bytes);
    if (NT_SUCCESS(status)) {
        status = xsdk_shader_load(&pixel_shader, blit_frag.container, blit_frag.container_bytes);
    }
    if (NT_SUCCESS(status)) {
        status = xsdk_shader_set_vertex_layout(&vertex_shader, quad_layout,
                                               sizeof quad_layout / sizeof quad_layout[0],
                                               (const uint32_t[]){ STRIDE_BYTES });
    }
    if (!NT_SUCCESS(status)) {
        printf("witchaven360: loading the shaders failed: 0x%08X\n", (unsigned)status);
        return -1;
    }

    quad_vertices = xsdk_gpu_alloc(VERTEX_ALLOC_BYTES);
    if (quad_vertices == NULL) {
        printf("witchaven360: xsdk_gpu_alloc failed\n");
        return -1;
    }

    status = xsdk_present_open(XSDK_DISPLAY_720P, BUFFERS, SWAP_INTERVAL, &display_width, &display_height);
    if (!NT_SUCCESS(status)) {
        printf("witchaven360: xsdk_present_open failed: 0x%08X\n", (unsigned)status);
        return -1;
    }

    xsdk_texture_sampler_state(&frame_sampler, &sampler);

    printf("witchaven360: display %ux%u\n", (unsigned)display_width, (unsigned)display_height);
    gpu_open = 1;
    return 0;
}

// Waits until the GPU has drawn every frame it was given, so its textures and the quad can change.
static void WaitForGPU(void)
{
    unsigned int i;

    for (i = 0; i < FRAME_TEXTURES; i++) {
        if (frame_fences[i] != 0) {
            const NTSTATUS status = xsdk_gpu_wait(frame_fences[i], FENCE_TIMEOUT_MS);
            if (!NT_SUCCESS(status)) {
                printf("witchaven360: xsdk_gpu_wait failed: 0x%08X\n", (unsigned)status);
            }
            frame_fences[i] = 0;
        }
    }
}

// The quad in the middle of the display, as two triangles in clip space (y up), v = 0 the frame's top row. 320x200
// and 640x400 are shown 4:3, their 200 (400) lines as 240 (480), as the PC showed them; any other mode with square
// pixels. Full height, unless that's wider than the display.
static void WriteQuad(int width, int height)
{
    int shown_height = height;
    int quad_width, quad_height;
    float half_width, half_height, l, r, t, b;

    if ((width == 320 && height == 200) || (width == 640 && height == 400)) {
        shown_height = 6 * height / 5;
    }

    quad_height = (int)display_height;
    quad_width = (int)display_height * width / shown_height;
    if (quad_width > (int)display_width) {
        quad_width = (int)display_width;
        quad_height = (int)display_width * shown_height / width;
    }

    printf("witchaven360: frame %dx%d, drawn %dx%d in the middle of %ux%u\n", width, height, quad_width, quad_height,
           (unsigned)display_width, (unsigned)display_height);

    half_width = (float)quad_width / (float)display_width;
    half_height = (float)quad_height / (float)display_height;
    l = -half_width; r = half_width; t = half_height; b = -half_height;

    {
        const float quad[QUAD_VERTICES][VERTEX_FLOATS] = {
            { l, t, 0.0f, 0.0f }, { r, t, 1.0f, 0.0f }, { l, b, 0.0f, 1.0f },
            { l, b, 0.0f, 1.0f }, { r, t, 1.0f, 0.0f }, { r, b, 1.0f, 1.0f },
        };

        WriteGPU(quad_vertices, quad, sizeof quad);
    }
}

static int MakeFrameTextures(int width, int height)
{
    unsigned int i;

    WaitForGPU();

    if (texture_width == width && texture_height == height) {
        WriteQuad(width, height);
        return 0;
    }

    for (i = 0; i < FRAME_TEXTURES; i++) {
        NTSTATUS status;

        if (texture_width != 0) {
            xsdk_texture_free(&frame_textures[i]);
        }

        status = xsdk_texture_create(&frame_textures[i], XSDK_FORMAT_A8R8G8B8, width, height, 1, true);
        if (!NT_SUCCESS(status)) {
            printf("witchaven360: xsdk_texture_create %dx%d failed: 0x%08X\n", width, height, (unsigned)status);
            texture_width = texture_height = 0;
            return -1;
        }
    }

    texture_width = width;
    texture_height = height;
    WriteQuad(width, height);
    return 0;
}

void getvalidmodes(void)
{
    if (validmodecnt) return;

    // 8-bit modes up to the display's 1280x720, "fullscreen" and "windowed" alike: they're shown the same.
    addstandardvalidmodes(MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, 8, 1, 0, 0, -1);
    addstandardvalidmodes(MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, 8, 0, 0, 0, -1);
    sortvalidmodes();
}

int setvideomode(int xdim, int ydim, int bitspp, int fullsc)
{
    int i, j, pitch;

    if ((fullsc == fullscreen) && (xdim == xres) && (ydim == yres) && (bitspp == bpp) && !videomodereset) {
        OSD_ResizeDisplay(xres, yres);
        return 0;
    }

    if (bitspp != 8 || xdim < 320 || ydim < 200 || xdim > MAX_FRAME_WIDTH || ydim > MAX_FRAME_HEIGHT) {
        printf("witchaven360: can't show a %dx%d %d-bit mode; 8-bit, 320x200 to %dx%d only\n", xdim, ydim, bitspp,
               MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT);
        return -1;
    }

    buildprintf("Setting video mode %dx%d (%d-bit)\n", xdim, ydim, bitspp);

    if (OpenGPU() < 0) {
        return -1;
    }

    if (baselayer_videomodewillchange) baselayer_videomodewillchange();

    if (MakeFrameTextures(xdim, ydim) < 0) {
        return -1;
    }

    // Rounded up to a multiple of 4, as the SDL2 layer has it.
    pitch = (((xdim | 1) + 4) & ~3);

    free(frame);
    free(argb_frame);
    frame = (unsigned char *)malloc(pitch * ydim);
    argb_frame = (uint32_t *)malloc(xdim * ydim * sizeof(uint32_t));
    if (!frame || !argb_frame) {
        buildputs("Unable to allocate framebuffer\n");
        return -1;
    }
    memset(frame, 0, pitch * ydim);

    frameplace = (intptr_t)frame;
    bytesperline = pitch;
    imageSize = bytesperline * ydim;
    numpages = 1;

    setvlinebpl(bytesperline);
    for (i = j = 0; i <= ydim; i++) {
        ylookup[i] = j;
        j += bytesperline;
    }

    xres = xdim;
    yres = ydim;
    bpp = bitspp;
    fullscreen = fullsc;

    videomodereset = 0;
    if (baselayer_videomodedidchange) baselayer_videomodedidchange();
    OSD_ResizeDisplay(xres, yres);

    // setpalettefade sets the palette according to whether gamma worked.
    setpalettefade(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

    startwin_close();

    return 0;
}

const char *getdisplayname(int display)
{
    return display == 0 ? "Xbox 360" : NULL;
}

void showframe(void)
{
    uint32_t address;
    NTSTATUS status;
    const unsigned int t = frame_count % FRAME_TEXTURES;
    XsdkTexture *const texture = &frame_textures[t];
    const unsigned char *in;
    uint32_t *out;
    int x, y;

    if (!gpu_open || gpu_failed || frame == NULL || texture_width != xres || texture_height != yres) {
        return;
    }

    // The palette the engine wants on the screen now, brightness and tints applied (curpalettefaded).
    for (x = 0; x < 256; x++) {
        palette_argb[x] = 0xFF000000u | ((uint32_t)curpalettefaded[x].r << 16) |
                          ((uint32_t)curpalettefaded[x].g << 8) | (uint32_t)curpalettefaded[x].b;
    }

    in = frame;
    out = argb_frame;
    for (y = 0; y < yres; y++) {
        for (x = 0; x < xres; x++) {
            out[x] = palette_argb[in[x]];
        }
        in += bytesperline;
        out += xres;
    }

    status = xsdk_present_begin(&address, BEGIN_TIMEOUT_MS);
    if (!NT_SUCCESS(status)) {
        // The GPU has hung: say so once, and stop drawing rather than wait 5 s a frame.
        printf("witchaven360: xsdk_present_begin failed: 0x%08X\n", (unsigned)status);
        FlushLog();
        gpu_failed = 1;
        return;
    }

    // The texture was last drawn two frames ago; wait until the GPU has finished with it before writing it again.
    if (frame_fences[t] != 0) {
        status = xsdk_gpu_wait(frame_fences[t], FENCE_TIMEOUT_MS);
        if (!NT_SUCCESS(status)) {
            printf("witchaven360: xsdk_gpu_wait failed: 0x%08X\n", (unsigned)status);
        }
    }

    xsdk_texture_upload(texture, 0, argb_frame, xres * sizeof(uint32_t));
    xsdk_gpu_invalidate(texture->address, texture->size_bytes);

    // A resolve leaves no target, shaders or texture slot 0 behind, so each frame sets them again.
    xsdk_gpu_set_target(display_width, display_height);
    xsdk_gpu_set_shaders(&vertex_shader, &pixel_shader);
    xsdk_built_shader_set_literals(&blit_vert);
    xsdk_built_shader_set_literals(&blit_frag);
    xsdk_gpu_set_texture(BLIT_FRAG_SAMPLER_0_0, texture, &frame_sampler);
    xsdk_gpu_set_vertices(0, xsdk_gpu_address(quad_vertices), QUAD_BYTES);
    xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_LIST, QUAD_VERTICES);
    // Copies the frame into the front buffer, then clears eDRAM to black for the next frame's bars.
    xsdk_gpu_resolve(address, display_width, display_height, CLEAR_ARGB);
    frame_fences[t] = xsdk_gpu_fence();

    xsdk_present_end();

    if (frame_count == 0) {
        printf("witchaven360: first frame presented\n");
    }

    frame_count++;
}

int setpalette(int start, int num, unsigned char *dapal)
{
    // showframe reads the engine's palette (curpalettefaded) itself each frame.
    (void)start; (void)num; (void)dapal;
    return 0;
}

int setsysgamma(float shadergamma, float sysgamma)
{
    // No system gamma on the console: the game's brightness goes through the palette.
    (void)shadergamma; (void)sysgamma;
    return -1;
}
