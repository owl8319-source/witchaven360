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
//
// With Settings > Vibration on, the controller rumbles as the game hits the player, as a weapon strikes, as a spell is
// cast and as something is picked up (wh360_rumble, called from whplr.c and whobj.c). Not in the menus, and not while
// the Guide is open.

#include "icorp.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>

#include "xenonlayer.h"
#include "wh_pad.h"

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
    "Run", "Centre view", "Map",
};
static const char *const action_ids[PA_COUNT] = {
    "nothing", "attack", "cast", "use", "jump", "crouch", "drink", "prevweapon", "nextweapon", "prevspell",
    "nextspell", "prevpotion", "nextpotion", "run", "centre", "map",
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
// Keys pressed for one update only (choosing a weapon or a spell), let go on the next.
static unsigned char keys_tapped[256];

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

// The buttons as the game starts; invert look and vibration stay as they are.
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

    f = fopen(LAYOUT_FILE, "r");
    if (!f) return;

    while (fgets(line, sizeof(line), f)) {
        int button = -1, action = -1;

        if (sscanf(line, " %63[a-z] = %63[a-z]", key, value) != 2) continue;
        if (!strcmp(key, "invertlook")) {
            invert_look = !strcmp(value, "on");
            continue;
        }
        if (!strcmp(key, "vibration")) {
            vibration = strcmp(value, "off") != 0;
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
    fclose(f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Keys

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
        case PA_PREV_WEAPON: if (Pressed(button)) ChangeWeapon(plr, -1); break;
        case PA_NEXT_WEAPON: if (Pressed(button)) ChangeWeapon(plr, 1); break;
        case PA_PREV_SPELL:  if (Pressed(button)) ChangeSpell(plr, -1); break;
        case PA_NEXT_SPELL:  if (Pressed(button)) ChangeSpell(plr, 1); break;
        default: break;
    }
}

static void GameControls(void)
{
    struct player *plr = &player[pyrn];
    int i;

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
        if (invert_look) joyaxis[3] = joyaxis[3] == -32768 ? 32767 : -joyaxis[3];
    } else {
        joyaxis[0] = joyaxis[1] = joyaxis[2] = joyaxis[3] = 0;
    }

    for (i = 0; i < PAD_BUTTONS; i++) {
        DoAction(plr, i, layout[i]);
    }
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

void xenon_pad_update(void)
{
    int code, in_game;

    if (!layout_loaded) xenon_pad_load_layout();

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
        GameControls();
    } else {
        MenuControls();
    }
}
