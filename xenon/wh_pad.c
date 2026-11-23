// Witchaven 360: the Xbox 360 controller, turned into the keys and stick movement Witchaven reads.
//
// The game reads the keyboard (keystatus[], through the keys[] the settings give each action) and, with its joystick
// on, the sticks (joyaxis[], whinp.c's getinput). So the controller's buttons become key presses, and the sticks go
// into joyaxis[]. Which keys depends on whether a game tic has read the controls lately (xenon_pad_gametic, from
// getinput): in the game each button does the action the button layout gives it (Settings > Button layout,
// xenon/wh_menu360.c, kept in controller.cfg beside the .xex, with Settings > Invert look); in the menus they're the
// arrow keys, Enter and Escape.
//
//   In the game (as it starts)                   In the menus
//   Left stick      move and strafe              Left stick, D-pad  up, down, left, right
//   Right stick     turn and look up or down     A                  select (Enter)
//   RT              attack                       B, Start           back (Escape)
//   LT              cast the chosen spell        Y                  settings (the main menu's "/")
//   A               use (doors, switches)
//   B               jump, or fly up
//   X               drink the chosen potion
//   Y               crouch, or fly down
//   LB / RB         previous / next weapon
//   D-pad left / right  previous / next spell
//   D-pad up / down     next / previous potion
//   Left stick click    run, while held
//   Right stick click   centre the view
//   Back            map
//   Start           menu (Escape), always
//   Back + D-pad up / down   quick save / quick load (the map then opens on letting Back go, not pressing it)
//   (End flying, the PC's End key, on no button unless given one; flying down to the ground lands too, whinp.c)
//   (Quick save and Quick load can be given buttons of their own too.)
//   LB and RB act 120 ms late, so pressing both for the developer menu doesn't change weapon first.
//
// With the map open, D-pad up and down zoom it in and out, whatever they do in the game.
//
// With Settings > Vibration on, the controller rumbles as the game hits the player, as a weapon strikes, as a spell is
// cast and as something is picked up (wh360_rumble, called from whplr.c and whobj.c). Not in the menus, and not while
// the Guide is open.

#include "icorp.h"

#include <SDL3/SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xenonlayer.h"
#include "wh_pad.h"
#include "wh_devmenu.h"

// Quick save and load are the game's (whmenu.c), done between its frames.
extern void wh360_quick_ask(int load);

extern void initjstick(void);
extern int selectedgun;
extern int currentorb;

// How long after the last game tic the controls still count as the game's: longer than a tic, short enough that a
// menu opened from the game gets the menu's buttons at once.
#define GAME_TIC_HOLD_MS 200
// A stick pushed past this works as a direction in the menus.
#define MENU_STICK_THRESHOLD (50 * 32768 / 100)
// A trigger pulled past this is held.
#define TRIGGER_THRESHOLD (30 * 32767 / 100)

#define LAYOUT_FILE "controller.cfg"

// The game's scan codes for the keys the menus read (raf.h's LUP and so on, and the keyboard's Enter, Space, Escape
// and "/").
#define SC_ESCAPE 0x01
#define SC_ENTER  0x1C
#define SC_SPACE  0x39
#define SC_SLASH  0x35

// The controller's buttons and triggers: the ones the layout gives actions (PAD_BUTTONS of them, wh_pad.h's order),
// then Start.
enum {
    PB_START = PAD_BUTTONS,
    PB_COUNT,
};

static const int sdl_buttons[PB_COUNT] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, -1, -1,
    SDL_GAMEPAD_BUTTON_BACK,
    SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
    SDL_GAMEPAD_BUTTON_START,
};

// The names the menus show, and the ones controller.cfg keeps.
static const char *const button_names[PAD_BUTTONS] = {
    "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "Back", "Left stick click", "Right stick click",
    "D-pad up", "D-pad down", "D-pad left", "D-pad right",
};
static const char *const button_ids[PAD_BUTTONS] = {
    "a", "b", "x", "y", "lb", "rb", "lt", "rt", "back", "lclick", "rclick", "dup", "ddown", "dleft", "dright",
};
static const char *const action_names[PA_COUNT] = {
    "Nothing", "Attack", "Cast spell", "Use", "Jump / fly up", "Crouch / fly down", "Drink potion",
    "Previous weapon", "Next weapon", "Previous spell", "Next spell", "Previous potion", "Next potion",
    "Run", "Centre view", "Map", "End flying", "Quick save", "Quick load",
};
static const char *const action_ids[PA_COUNT] = {
    "nothing", "attack", "cast", "use", "jump", "crouch", "drink", "prevweapon", "nextweapon", "prevspell",
    "nextspell", "prevpotion", "nextpotion", "run", "centre", "map", "endfly", "quicksave", "quickload",
};

static const int default_layout[PAD_BUTTONS] = {
    PA_USE,             // A
    PA_JUMP,            // B
    PA_DRINK,           // X
    PA_CROUCH,          // Y
    PA_PREV_WEAPON,     // LB
    PA_NEXT_WEAPON,     // RB
    PA_CAST,            // LT
    PA_ATTACK,          // RT
    PA_MAP,             // Back
    PA_RUN,             // left stick click
    PA_CENTRE,          // right stick click
    PA_NEXT_POTION,     // D-pad up
    PA_PREV_POTION,     // D-pad down
    PA_PREV_SPELL,      // D-pad left
    PA_NEXT_SPELL,      // D-pad right
};

static int layout[PAD_BUTTONS];
static int layout_loaded = 0;
// The right stick's up and down the other way round (Settings > Invert look).
static int invert_look = 0;
// Settings > Vibration.
static int vibration = 1;
// Settings > Turn speed and Look speed, 1 to 10: how fast the right stick turns, across, and looks up and down; 7 as the
// game had it (as Heretic 360 has them).
static int turn_speed = DEFAULT_TURN_SPEED;
static int look_speed = DEFAULT_LOOK_SPEED;
// Settings > Stick dead zone, 0 to 30 per cent of the sticks' travel, by 5s: how far they go before they move.
static int dead_zone = DEFAULT_DEAD_ZONE;
// Settings > Status bar (on, or off for the view to fill the screen; whaven.c), and Settings > Smoothing (0 off,
// 1 smooth, 2 sharp; xenonlayer.c, blit.frag). Not the controller's, but kept with its settings, in controller.cfg.
static int status_bar = 1;
static int smoothing = 0;
// The rumble going now: how strong, and until when (getticks()); rumble_until 0 when none.
static int rumble_strength = 0;
static unsigned int rumble_until = 0;

static unsigned int last_game_tic_ms;
static int had_game_tic = 0;
static int joystick_on = 0;
static int last_in_game = -1;

static int held[PB_COUNT];
static int was_held[PB_COUNT];

// The keys the controller is holding down, so they can be let go.
static unsigned char keys_down[256];
// Keys pressed only until the game has read them once (choosing a weapon or a spell): let go once a game tic has run
// since (game_tics past the number kept here), or after TAP_LONGEST_MS without one. The game reads them in its tics
// (whinp.c's getinput), not every frame, so a key let go on the next frame could be missed.
static unsigned int keys_tapped[256];
static unsigned int keys_tapped_ms[256];
static unsigned int game_tics = 0;
#define TAP_LONGEST_MS 250

// Menu directions from the left stick, as held buttons.
static int stick_up, stick_down, stick_left, stick_right;

// ---------------------------------------------------------------------------------------------------------------------
// The layout

const char *xenon_pad_button_name(int button)
{
    return (button >= 0 && button < PAD_BUTTONS) ? button_names[button] : "";
}

const char *xenon_pad_action_name(int action)
{
    return (action >= 0 && action < PA_COUNT) ? action_names[action] : "";
}

int xenon_pad_action(int button)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return (button >= 0 && button < PAD_BUTTONS) ? layout[button] : PA_NOTHING;
}

void xenon_pad_set_action(int button, int action)
{
    if (!layout_loaded) xenon_pad_load_layout();
    if (button >= 0 && button < PAD_BUTTONS && action >= 0 && action < PA_COUNT) layout[button] = action;
}

int xenon_pad_invert_look(void)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return invert_look;
}

void xenon_pad_set_invert_look(int on)
{
    if (!layout_loaded) xenon_pad_load_layout();
    invert_look = on ? 1 : 0;
}

int xenon_pad_vibration(void)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return vibration;
}

static void StartRumble(int strength, int ms)
{
    rumble_strength = strength;
    rumble_until = getticks() + (unsigned int)ms;
    if (rumble_until == 0) rumble_until = 1;
    // The heavy motor (left) at the strength, the light one (right) a little under, as Heretic 360 has it.
    xenon_pad_rumble(strength * 65535 / 100, strength * 65535 / 160, ms);
}

static void StopRumble(void)
{
    if (rumble_until == 0) return;
    xenon_pad_rumble(0, 0, 0);
    rumble_until = 0;
    rumble_strength = 0;
}

void xenon_pad_set_vibration(int on)
{
    if (!layout_loaded) xenon_pad_load_layout();
    vibration = on ? 1 : 0;
    if (vibration) {
        // A short one, so turning it on can be felt.
        StartRumble(40, 200);
    } else {
        StopRumble();
    }
}

void wh360_rumble(int strength, int ms)
{
    if (!layout_loaded) xenon_pad_load_layout();
    if (!vibration || strength <= 0 || ms <= 0 || last_in_game != 1 || xenon_system_ui_open()) return;
    if (strength > 100) strength = 100;
    if (rumble_until != 0 && (int)(rumble_until - getticks()) > 0 && strength <= rumble_strength) return;
    StartRumble(strength, ms);
}

int xenon_pad_turn_speed(void)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return turn_speed;
}

void xenon_pad_set_turn_speed(int value)
{
    if (!layout_loaded) xenon_pad_load_layout();
    turn_speed = value < 1 ? 1 : value > 10 ? 10 : value;
}

int xenon_pad_look_speed(void)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return look_speed;
}

void xenon_pad_set_look_speed(int value)
{
    if (!layout_loaded) xenon_pad_load_layout();
    look_speed = value < 1 ? 1 : value > 10 ? 10 : value;
}

int xenon_pad_dead_zone(void)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return dead_zone;
}

void xenon_pad_set_dead_zone(int percent)
{
    if (!layout_loaded) xenon_pad_load_layout();
    dead_zone = percent < 0 ? 0 : percent > 30 ? 30 : percent / 5 * 5;
}

int xenon_pad_status_bar(void)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return status_bar;
}

void xenon_pad_set_status_bar(int on)
{
    if (!layout_loaded) xenon_pad_load_layout();
    status_bar = on ? 1 : 0;
}

int xenon_pad_smoothing(void)
{
    if (!layout_loaded) xenon_pad_load_layout();
    return smoothing;
}

void xenon_pad_set_smoothing(int mode)
{
    if (!layout_loaded) xenon_pad_load_layout();
    smoothing = mode < 0 ? 0 : mode > 2 ? 2 : mode;
}

// The buttons as the game starts; the other settings stay as they are.
void xenon_pad_reset_layout(void)
{
    memcpy(layout, default_layout, sizeof(layout));
    layout_loaded = 1;
}

void xenon_pad_load_layout(void)
{
    FILE *f;
    char line[128], key[64], value[64];
    int i;

    xenon_pad_reset_layout();
    invert_look = 0;
    vibration = 1;
    turn_speed = DEFAULT_TURN_SPEED;
    look_speed = DEFAULT_LOOK_SPEED;
    dead_zone = DEFAULT_DEAD_ZONE;
    status_bar = 1;
    smoothing = 0;

    f = fopen(LAYOUT_FILE, "r");
    if (!f) return;

    while (fgets(line, sizeof(line), f)) {
        int button = -1, action = -1;

        if (sscanf(line, " %63[a-z] = %63[a-z0-9]", key, value) != 2) continue;
        if (!strcmp(key, "invertlook")) {
            invert_look = !strcmp(value, "on");
            continue;
        }
        if (!strcmp(key, "vibration")) {
            vibration = strcmp(value, "off") != 0;
            continue;
        }
        if (!strcmp(key, "turnspeed")) {
            xenon_pad_set_turn_speed(atoi(value));
            continue;
        }
        if (!strcmp(key, "lookspeed")) {
            xenon_pad_set_look_speed(atoi(value));
            continue;
        }
        if (!strcmp(key, "deadzone")) {
            xenon_pad_set_dead_zone(atoi(value));
            continue;
        }
        if (!strcmp(key, "statusbar")) {
            status_bar = strcmp(value, "off") != 0;
            continue;
        }
        if (!strcmp(key, "smoothing")) {
            smoothing = !strcmp(value, "smooth") ? 1 : !strcmp(value, "sharp") ? 2 : 0;
            continue;
        }
        for (i = 0; i < PAD_BUTTONS; i++) if (!strcmp(key, button_ids[i])) button = i;
        for (i = 0; i < PA_COUNT; i++) if (!strcmp(value, action_ids[i])) action = i;
        if (button >= 0 && action >= 0) layout[button] = action;
    }

    fclose(f);
    printf("witchaven360: button layout from " LAYOUT_FILE "\n");
}

void xenon_pad_save_layout(void)
{
    FILE *f = fopen(LAYOUT_FILE, "w");
    int i;

    if (!f) {
        printf("witchaven360: couldn't write " LAYOUT_FILE "\n");
        return;
    }

    fprintf(f, "; Witchaven 360's button layout: Settings > Button layout. Start always opens the menu.\n");
    for (i = 0; i < PAD_BUTTONS; i++) fprintf(f, "%s = %s\n", button_ids[i], action_ids[layout[i]]);
    fprintf(f, "invertlook = %s\n", invert_look ? "on" : "off");
    fprintf(f, "vibration = %s\n", vibration ? "on" : "off");
    fprintf(f, "turnspeed = %d\n", turn_speed);
    fprintf(f, "lookspeed = %d\n", look_speed);
    fprintf(f, "deadzone = %d\n", dead_zone);
    fprintf(f, "statusbar = %s\n", status_bar ? "on" : "off");
    fprintf(f, "smoothing = %s\n", smoothing == 1 ? "smooth" : smoothing == 2 ? "sharp" : "off");
    fclose(f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Keys

void xenon_pad_gametic(void)
{
    last_game_tic_ms = getticks();
    had_game_tic = 1;
    game_tics++;
}

static void KeyDown(int code)
{
    if (code <= 0 || code > 255 || keys_down[code]) return;
    keys_down[code] = 1;
    xenon_keyevent(code, 1);
}

static void KeyUp(int code)
{
    if (code <= 0 || code > 255 || !keys_down[code]) return;
    keys_down[code] = 0;
    xenon_keyevent(code, 0);
}

static void KeyTap(int code)
{
    if (code <= 0 || code > 255) return;
    KeyDown(code);
    keys_tapped[code] = game_tics + 1;  // 0 is none
    keys_tapped_ms[code] = getticks();
}

// A key held while the button is: down as it's pressed, up as it's let go.
static void HoldKey(int button, int code)
{
    if (held[button] && !was_held[button]) KeyDown(code);
    else if (!held[button] && was_held[button]) KeyUp(code);
}

static int Pressed(int button)
{
    return held[button] && !was_held[button];
}

void xenon_releaseallkeys(void)
{
    int code;

    for (code = 1; code < 256; code++) {
        KeyUp(code);
        keys_tapped[code] = 0;
    }
}

static void ReadButtons(void)
{
    int i;

    memcpy(was_held, held, sizeof(held));

    if (!xenon_pad_connected() || xenon_system_ui_open()) {
        // While the Guide or a dialog of the console's is up, the controller reads as let go.
        memset(held, 0, sizeof(held));
        return;
    }

    for (i = 0; i < PB_COUNT; i++) {
        if (sdl_buttons[i] >= 0) held[i] = xenon_pad_button(sdl_buttons[i]);
    }
    held[PAD_LT] = xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > TRIGGER_THRESHOLD;
    held[PAD_RT] = xenon_pad_axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > TRIGGER_THRESHOLD;
}

// The next (dir 1) or previous (-1) weapon the player has, from the one in hand, chosen with its number key.
static void ChangeWeapon(struct player *plr, int dir)
{
    int i, w = selectedgun;

    for (i = 0; i < MAXWEAPONS; i++) {
        w = (w + dir + MAXWEAPONS) % MAXWEAPONS;
        if (plr->weapon[w] > 0) {
            if (w != selectedgun) KeyTap(keys[KEYWEAP1 + w]);
            return;
        }
    }
}

// The next or previous spell the player has, chosen with its key.
static void ChangeSpell(struct player *plr, int dir)
{
    int i, s = currentorb;

    for (i = 0; i < MAXNUMORBS; i++) {
        s = (s + dir + MAXNUMORBS) % MAXNUMORBS;
        if (plr->orb[s] > 0) {
            KeyTap(keys[KEYSPELL1 + s]);
            return;
        }
    }
}

// What a button does in the game, as the layout has it.
static void DoAction(struct player *plr, int button, int action)
{
    switch (action) {
        case PA_ATTACK:      HoldKey(button, keys[KEYFIRE]); break;
        case PA_CAST:        HoldKey(button, keys[KEYCAST]); break;
        case PA_USE:         HoldKey(button, keys[KEYUSE]); break;
        // Jumping does nothing while flying, and flying up nothing on the ground, so one button does both.
        case PA_JUMP:        HoldKey(button, keys[KEYJUMP]); HoldKey(button, keys[KEYFLYUP]); break;
        case PA_CROUCH:      HoldKey(button, keys[KEYCROUCH]); HoldKey(button, keys[KEYFLYDN]); break;
        case PA_DRINK:       HoldKey(button, keys[KEYUSEP]); break;
        case PA_PREV_POTION: HoldKey(button, keys[KEYPREVP]); break;
        case PA_NEXT_POTION: HoldKey(button, keys[KEYNEXTP]); break;
        case PA_RUN:         HoldKey(button, keys[KEYRUN]); break;
        case PA_CENTRE:      HoldKey(button, keys[KEYCNTR]); break;
        case PA_MAP:         HoldKey(button, keys[KEYMAP]); break;
        case PA_END_FLYING:  HoldKey(button, keys[KEYUNCAST]); break;
        case PA_PREV_WEAPON: if (Pressed(button)) ChangeWeapon(plr, -1); break;
        case PA_NEXT_WEAPON: if (Pressed(button)) ChangeWeapon(plr, 1); break;
        case PA_PREV_SPELL:  if (Pressed(button)) ChangeSpell(plr, -1); break;
        case PA_NEXT_SPELL:  if (Pressed(button)) ChangeSpell(plr, 1); break;
        case PA_QUICK_SAVE:  if (Pressed(button)) wh360_quick_ask(0); break;
        case PA_QUICK_LOAD:  if (Pressed(button)) wh360_quick_ask(1); break;
        default: break;
    }
}

static int zoom_in_down = 0, zoom_out_down = 0;


// LB and RB: how long one waits before what it does in the game happens, so that pressing both together, to open the
// developer menu with Back, doesn't change weapon twice first. Pressed apart, each happens SHOULDER_WAIT_MS late (or
// as it's let go, if sooner); pressed within that of each other, neither happens.
#define SHOULDER_WAIT_MS 120

typedef struct {
    unsigned int since;  // getticks() when pressed, 0 when not held
    int cancelled;       // pressed with the other: does nothing until let go
    int done;            // has gone down to the game this press
    int was;             // what the game was given last time
} shoulder_t;

static shoulder_t shoulders[2];

// What the game's given for shoulder button 'b' (PAD_LB or PAD_RB), 1 or 0.
static int ShoulderHeld(int b, unsigned int now)
{
    shoulder_t *s = &shoulders[b == PAD_RB];
    shoulder_t *other = &shoulders[b != PAD_RB];
    int give = 0;

    if (held[b]) {
        if (s->since == 0) {
            s->since = now ? now : 1;
            s->cancelled = s->done = 0;
        }
        if (!s->done && other->since != 0 && !other->done &&
            (s->since > other->since ? s->since - other->since : other->since - s->since) < SHOULDER_WAIT_MS) {
            s->cancelled = other->cancelled = 1;
        }
        give = !s->cancelled && (s->done || now - s->since >= SHOULDER_WAIT_MS);
    } else if (s->since != 0) {
        // Let go before its wait was up: a tap, given now, let go of next time.
        give = !s->cancelled && !s->done;
        s->since = 0;
    }
    if (give) s->done = 1;
    return give;
}

// Back, while it's the map's: the map on letting it go, unless D-pad up (quick save) or down (quick load) was pressed
// with it, or LB and RB were held for the developer menu.
static unsigned int back_since = 0;
static int back_used = 0;
static int back_was = 0;

static int BackHeld(void)
{
    int give = 0;

    if (held[PAD_BACK]) {
        if (back_since == 0) {
            back_since = 1;
            back_used = 0;
        }
        if (held[PAD_LB] && held[PAD_RB]) back_used = 1;
        if (held[PAD_DUP] && !was_held[PAD_DUP]) {
            wh360_quick_ask(0);
            back_used = 1;
        }
        if (held[PAD_DDOWN] && !was_held[PAD_DDOWN]) {
            wh360_quick_ask(1);
            back_used = 1;
        }
    } else if (back_since != 0) {
        give = !back_used;
        back_since = 0;
    }
    return give;
}

// A stick, from -32768 to 32767 each way, as the game takes it: nothing within the dead zone, all round (a circle,
// not a square, so a diagonal isn't cut short); from its edge to full tilt, 0 to 32767. The right stick's 'curved',
// the square of how far it's pushed, so a small push turns slowly and full tilt still turns fast, then across times
// 'gain_x' and up and down times 'gain_y', Settings > Turn speed's and Look speed's (0.4 to 1.3, 1 as the game had it).
static void Stick(int x, int y, int curved, float gain_x, float gain_y, int *out_x, int *out_y)
{
    const float edge = (float)dead_zone * 32767.0f / 100.0f;
    const float fx = (float)x, fy = (float)y;
    const float length = sqrtf(fx * fx + fy * fy);
    float amount, scale, ox, oy;

    if (length <= edge || length < 1.0f) {
        *out_x = *out_y = 0;
        return;
    }

    amount = (length - edge) / (32767.0f - edge);
    if (amount > 1.0f) amount = 1.0f;
    if (curved) amount *= amount;
    scale = amount * 32767.0f / length;
    ox = fx * scale * gain_x;
    oy = fy * scale * gain_y;
    *out_x = ox > 32767.0f ? 32767 : ox < -32767.0f ? -32767 : (int)ox;
    *out_y = oy > 32767.0f ? 32767 : oy < -32767.0f ? -32767 : (int)oy;
}

// A key held while 'down', pressed and let go of as that changes.
static void SetZoom(int code, int *state, int down)
{
    if (down == *state) return;
    if (down) KeyDown(code);
    else KeyUp(code);
    *state = down;
}

static void GameControls(void)
{
    struct player *plr = &player[pyrn];
    const unsigned int now = getticks();
    int i, map_open, back_is_map;

    if (!joystick_on) {
        // The game reads the sticks only with its joystick on (whaven.c's option[3]).
        initjstick();
        joystick_on = 1;
    }

    if (xenon_pad_connected() && !xenon_system_ui_open()) {
        // getinput takes them as they are (whinp.c): the dead zone and the right stick's curve and sensitivity are
        // here.
        Stick(xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFTX), xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFTY), 0, 1.0f, 1.0f,
              &joyaxis[0], &joyaxis[1]);
        Stick(xenon_pad_axis(SDL_GAMEPAD_AXIS_RIGHTX), xenon_pad_axis(SDL_GAMEPAD_AXIS_RIGHTY), 1,
              0.3f + 0.1f * (float)turn_speed, 0.3f + 0.1f * (float)look_speed, &joyaxis[2], &joyaxis[3]);
        if (invert_look) joyaxis[3] = -joyaxis[3];
    } else {
        joyaxis[0] = joyaxis[1] = joyaxis[2] = joyaxis[3] = 0;
    }

    // With the map open, D-pad up and down zoom it in and out (the PC's + and -) in place of what they do in the game.
    map_open = plr->dimension == 2;
    back_is_map = layout[PAD_BACK] == PA_MAP;
    for (i = 0; i < PAD_BUTTONS; i++) {
        // LB and RB a moment late (ShoulderHeld), and Back, as the map, on letting it go (BackHeld): given to the
        // game as pressed and let go of like any other button.
        if (i == PAD_LB || i == PAD_RB || (i == PAD_BACK && back_is_map)) {
            const int h = held[i], w = was_held[i];
            int *was = i == PAD_BACK ? &back_was : &shoulders[i == PAD_RB].was;
            held[i] = i == PAD_BACK ? BackHeld() : ShoulderHeld(i, now);
            was_held[i] = *was;
            DoAction(plr, i, layout[i]);
            *was = held[i];
            held[i] = h;
            was_held[i] = w;
            continue;
        }
        // With Back held (the map's), D-pad up and down are quick save and quick load (BackHeld).
        if (back_is_map && (i == PAD_DUP || i == PAD_DDOWN) && (held[PAD_BACK] || was_held[PAD_BACK]) &&
            held[i]) {
            continue;
        }
        if (map_open && (i == PAD_DUP || i == PAD_DDOWN)) {
            if (was_held[i]) {
                // Let go of what it does in the game, if it was held as the map opened.
                const int h = held[i];
                held[i] = 0;
                DoAction(plr, i, layout[i]);
                held[i] = h;
            }
            continue;
        }
        DoAction(plr, i, layout[i]);
    }
    SetZoom(keys[KEYZOOMI], &zoom_in_down, map_open && held[PAD_DUP]);
    SetZoom(keys[KEYZOOMO], &zoom_out_down, map_open && held[PAD_DDOWN]);
    HoldKey(PB_START, SC_ESCAPE);
}

static void MenuStick(void)
{
    int x = 0, y = 0;

    if (xenon_pad_connected() && !xenon_system_ui_open()) {
        x = xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFTX);
        y = xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFTY);
    }

    stick_up = y < -MENU_STICK_THRESHOLD;
    stick_down = y > MENU_STICK_THRESHOLD;
    stick_left = x < -MENU_STICK_THRESHOLD;
    stick_right = x > MENU_STICK_THRESHOLD;
}

static void SetKey(int code, int down)
{
    if (down) KeyDown(code);
    else KeyUp(code);
}

static void MenuControls(void)
{
    joyaxis[0] = joyaxis[1] = joyaxis[2] = joyaxis[3] = 0;

    MenuStick();

    SetKey(LUP, held[PAD_DUP] || stick_up);
    SetKey(LDN, held[PAD_DDOWN] || stick_down);
    SetKey(LLEFT, held[PAD_DLEFT] || stick_left);
    SetKey(LRIGHT, held[PAD_DRIGHT] || stick_right);

    // Enter selects; some screens ("press a key") take Space instead.
    HoldKey(PAD_A, SC_ENTER);
    HoldKey(PAD_A, SC_SPACE);
    // Y opens the settings from the main menu ("Press Y for settings").
    HoldKey(PAD_Y, SC_SLASH);
    // B and Start go back.
    SetKey(SC_ESCAPE, held[PAD_B] || held[PB_START]);
}

// The developer cheat menu (wh_devmenu.c): LB + RB + Back held this long in a level opens it.
#define DEVMENU_HOLD_MS 1000
static unsigned int devmenu_combo_since = 0;
static int devmenu_combo_used = 0;
static int devmenu_stick_was = 0;

// Watches for LB + RB + Back held in a level, and runs the developer menu while it's open: D-pad or left stick up and
// down choose, A selects, B closes. True while it's open, the game's controls then doing nothing.
static int DevMenuControls(void)
{
    const int combo = held[PAD_LB] && held[PAD_RB] && held[PAD_BACK];
    const unsigned int now = getticks();
    int up, down;

    if (!combo) {
        devmenu_combo_since = 0;
        devmenu_combo_used = 0;
    } else if (devmenu_combo_since == 0) {
        devmenu_combo_since = now ? now : 1;
    } else if (!devmenu_combo_used && !wh360_devmenu_active() && now - devmenu_combo_since >= DEVMENU_HOLD_MS) {
        devmenu_combo_used = 1;
        xenon_releaseallkeys();
        zoom_in_down = zoom_out_down = 0;
        StopRumble();
        wh360_devmenu_open();
        memcpy(was_held, held, sizeof(held));
        return 1;
    }

    if (!wh360_devmenu_active()) return 0;

    // The game's controls don't run while it's open: forget LB, RB and Back's presses, as let go.
    memset(shoulders, 0, sizeof(shoulders));
    back_since = 0;
    back_was = 0;

    MenuStick();
    up = (held[PAD_DUP] && !was_held[PAD_DUP]) || (stick_up && !devmenu_stick_was);
    down = (held[PAD_DDOWN] && !was_held[PAD_DDOWN]) || (stick_down && !devmenu_stick_was);
    devmenu_stick_was = stick_up || stick_down;
    wh360_devmenu_input(up, down, held[PAD_A] && !was_held[PAD_A], held[PAD_B] && !was_held[PAD_B]);
    memcpy(was_held, held, sizeof(held));
    // Closed: back in the game, whose tics haven't run while it was open, so B isn't taken for the menus' Escape.
    if (!wh360_devmenu_active()) last_game_tic_ms = getticks();
    return 1;
}

void xenon_pad_update(void)
{
    int code, in_game;

    if (!layout_loaded) xenon_pad_load_layout();

    // Keys tapped go up once the game has read them.
    for (code = 1; code < 256; code++) {
        if (keys_tapped[code] && (game_tics + 1 > keys_tapped[code] ||
                                  getticks() - keys_tapped_ms[code] >= TAP_LONGEST_MS)) {
            keys_tapped[code] = 0;
            KeyUp(code);
        }
    }

    ReadButtons();

    // The developer menu stops the game's tics, but the game's still on screen behind it.
    in_game = wh360_devmenu_active() || (had_game_tic && (getticks() - last_game_tic_ms) < GAME_TIC_HOLD_MS);
    if (in_game != last_in_game) {
        // Between the game and the menus, let every key go, so nothing stays held from the other side. The buttons
        // still held count as pressed again on the new side only once they're let go and pressed.
        xenon_releaseallkeys();
        zoom_in_down = zoom_out_down = 0;
        memset(shoulders, 0, sizeof(shoulders));
        back_since = 0;
        back_was = 0;
        memcpy(was_held, held, sizeof(held));
        last_in_game = in_game;
        // And a rumble from the game stops as the menus open.
        StopRumble();
    }

    // A rumble stops as the Guide opens, and is forgotten once it has run its time. (Settings > Vibration's own short
    // one, in the menus, runs its time.)
    if (rumble_until != 0) {
        if (xenon_system_ui_open()) {
            StopRumble();
        } else if ((int)(rumble_until - getticks()) <= 0) {
            rumble_until = 0;
            rumble_strength = 0;
        }
    }

    if (in_game) {
        if (!DevMenuControls()) GameControls();
    } else {
        MenuControls();
    }
}
