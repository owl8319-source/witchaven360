// Witchaven 360: the Xbox 360 controller, turned into the keys and stick movement Witchaven reads.
//
// The game reads the keyboard (keystatus[], through the keys[] the settings give each action) and, with its joystick
// on, the sticks (joyaxis[], whinp.c's getinput). So the controller's buttons become key presses, and the sticks go
// into joyaxis[]. Which keys depends on whether a game tic has read the controls lately (xenon_pad_gametic, from
// getinput): in the game the buttons are actions, in the menus they're the arrow keys, Enter and Escape.
//
//   In the game                                  In the menus
//   Left stick      move and strafe              Left stick, D-pad  up, down, left, right
//   Right stick     turn and look up or down     A                  select (Enter)
//   RT              attack                       B, Start           back (Escape)
//   LT              cast the chosen spell        Y                  settings (the main menu's "/")
//   A               use (doors, switches)
//   B               jump
//   X               drink the chosen potion
//   Y               crouch
//   LB / RB         previous / next weapon
//   D-pad left / right  previous / next spell
//   D-pad up / down     next / previous potion
//   Left stick click    run, while held
//   Right stick click   centre the view
//   Back            map
//   Start           menu (Escape)

#include "icorp.h"

#include <SDL3/SDL.h>

#include "xenonlayer.h"

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

// The game's scan codes for the keys the menus read (raf.h's LUP and so on, and the keyboard's Enter, Space and Escape).
#define SC_ESCAPE 0x01
#define SC_ENTER  0x1C
#define SC_SPACE  0x39
#define SC_SLASH  0x35

enum {
    // The controller's buttons and triggers, as this file numbers them.
    PB_A, PB_B, PB_X, PB_Y, PB_LB, PB_RB, PB_LT, PB_RT, PB_BACK, PB_START, PB_LSTICK, PB_RSTICK,
    PB_DPAD_UP, PB_DPAD_DOWN, PB_DPAD_LEFT, PB_DPAD_RIGHT,
    PB_COUNT,
};

static const int sdl_buttons[PB_COUNT] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, -1, -1,
    SDL_GAMEPAD_BUTTON_BACK, SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
};

static unsigned int last_game_tic_ms;
static int had_game_tic = 0;
static int joystick_on = 0;
static int last_in_game = -1;

static int held[PB_COUNT];
static int was_held[PB_COUNT];

// The keys the controller is holding down, so they can be let go.
static unsigned char keys_down[256];
// Keys pressed for one update only (choosing a weapon or a spell), let go on the next.
static unsigned char keys_tapped[256];

// Menu directions from the left stick, as held buttons.
static int stick_up, stick_down, stick_left, stick_right;

void xenon_pad_gametic(void)
{
    last_game_tic_ms = getticks();
    had_game_tic = 1;
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
    keys_tapped[code] = 1;
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
    held[PB_LT] = xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > TRIGGER_THRESHOLD;
    held[PB_RT] = xenon_pad_axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > TRIGGER_THRESHOLD;
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

static void GameControls(void)
{
    struct player *plr = &player[pyrn];

    if (!joystick_on) {
        // The game reads the sticks only with its joystick on (whaven.c's option[3]).
        initjstick();
        joystick_on = 1;
    }

    if (xenon_pad_connected() && !xenon_system_ui_open()) {
        joyaxis[0] = xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFTX);
        joyaxis[1] = xenon_pad_axis(SDL_GAMEPAD_AXIS_LEFTY);
        joyaxis[2] = xenon_pad_axis(SDL_GAMEPAD_AXIS_RIGHTX);
        joyaxis[3] = xenon_pad_axis(SDL_GAMEPAD_AXIS_RIGHTY);
    } else {
        joyaxis[0] = joyaxis[1] = joyaxis[2] = joyaxis[3] = 0;
    }

    HoldKey(PB_RT, keys[KEYFIRE]);
    HoldKey(PB_LT, keys[KEYCAST]);
    HoldKey(PB_A, keys[KEYUSE]);
    HoldKey(PB_B, keys[KEYJUMP]);
    HoldKey(PB_X, keys[KEYUSEP]);
    HoldKey(PB_Y, keys[KEYCROUCH]);
    HoldKey(PB_LSTICK, keys[KEYRUN]);
    HoldKey(PB_RSTICK, keys[KEYCNTR]);
    HoldKey(PB_BACK, keys[KEYMAP]);
    HoldKey(PB_START, SC_ESCAPE);
    HoldKey(PB_DPAD_UP, keys[KEYNEXTP]);
    HoldKey(PB_DPAD_DOWN, keys[KEYPREVP]);

    if (Pressed(PB_LB)) ChangeWeapon(plr, -1);
    if (Pressed(PB_RB)) ChangeWeapon(plr, 1);
    if (Pressed(PB_DPAD_LEFT)) ChangeSpell(plr, -1);
    if (Pressed(PB_DPAD_RIGHT)) ChangeSpell(plr, 1);
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

    SetKey(LUP, held[PB_DPAD_UP] || stick_up);
    SetKey(LDN, held[PB_DPAD_DOWN] || stick_down);
    SetKey(LLEFT, held[PB_DPAD_LEFT] || stick_left);
    SetKey(LRIGHT, held[PB_DPAD_RIGHT] || stick_right);

    // Enter selects; some screens ("press a key") take Space instead.
    HoldKey(PB_A, SC_ENTER);
    HoldKey(PB_A, SC_SPACE);
    // Y opens the settings from the main menu ("Press / for settings").
    HoldKey(PB_Y, SC_SLASH);
    // B and Start go back.
    SetKey(SC_ESCAPE, held[PB_B] || held[PB_START]);
}

void xenon_pad_update(void)
{
    int code, in_game;

    // Keys tapped last time go up now.
    for (code = 1; code < 256; code++) {
        if (keys_tapped[code]) {
            keys_tapped[code] = 0;
            KeyUp(code);
        }
    }

    ReadButtons();

    in_game = had_game_tic && (getticks() - last_game_tic_ms) < GAME_TIC_HOLD_MS;
    if (in_game != last_in_game) {
        // Between the game and the menus, let every key go, so nothing stays held from the other side. The buttons
        // still held count as pressed again on the new side only once they're let go and pressed.
        xenon_releaseallkeys();
        memcpy(was_held, held, sizeof(held));
        last_in_game = in_game;
    }

    if (in_game) {
        GameControls();
    } else {
        MenuControls();
    }
}
