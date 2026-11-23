// Witchaven 360: the controller's button layout (wh_pad.c), for the Settings pages (wh_menu360.c).

#ifndef WH_PAD_H
#define WH_PAD_H

// The buttons the layout gives actions, in the order the Button layout page lists them. Start isn't one: it always
// opens the menu.
enum {
    PAD_A, PAD_B, PAD_X, PAD_Y, PAD_LB, PAD_RB, PAD_LT, PAD_RT, PAD_BACK, PAD_LCLICK, PAD_RCLICK,
    PAD_DUP, PAD_DDOWN, PAD_DLEFT, PAD_DRIGHT,
    PAD_BUTTONS,
};

// What a button can do in the game.
enum {
    PA_NOTHING, PA_ATTACK, PA_CAST, PA_USE, PA_JUMP, PA_CROUCH, PA_DRINK,
    PA_PREV_WEAPON, PA_NEXT_WEAPON, PA_PREV_SPELL, PA_NEXT_SPELL, PA_PREV_POTION, PA_NEXT_POTION,
    PA_RUN, PA_CENTRE, PA_MAP,
    PA_END_FLYING,
    PA_QUICK_SAVE,
    PA_QUICK_LOAD,
    PA_COUNT,
};

const char *xenon_pad_button_name(int button);
const char *xenon_pad_action_name(int action);
int xenon_pad_action(int button);
void xenon_pad_set_action(int button, int action);
int xenon_pad_invert_look(void);
void xenon_pad_set_invert_look(int on);
// Vibration (Settings > Vibration), kept in controller.cfg with the layout; on as the game starts.
int xenon_pad_vibration(void);
void xenon_pad_set_vibration(int on);
// A rumble of the controller: strength 0 to 100, for ms milliseconds. Only while playing and with vibration on; a
// weaker or equal one doesn't restart or cut short one still going (the game asks again each tic of a swing).
void wh360_rumble(int strength, int ms);
// Settings > Turn speed and Look speed (1 to 10; 7 turns and looks as the game did) and Settings > Stick dead zone
// (0 to 30 per cent, by 5s), kept in controller.cfg.
#define DEFAULT_TURN_SPEED 6
#define DEFAULT_LOOK_SPEED 4
#define DEFAULT_DEAD_ZONE 15
int xenon_pad_turn_speed(void);
void xenon_pad_set_turn_speed(int value);
int xenon_pad_look_speed(void);
void xenon_pad_set_look_speed(int value);
int xenon_pad_dead_zone(void);
void xenon_pad_set_dead_zone(int percent);
// Settings > Status bar (on, or off for a view filling the screen) and Settings > Smoothing (0 off, 1 smooth,
// 2 sharp), kept in controller.cfg too.
int xenon_pad_status_bar(void);
void xenon_pad_set_status_bar(int on);
int xenon_pad_smoothing(void);
void xenon_pad_set_smoothing(int mode);
void xenon_pad_reset_layout(void);
void xenon_pad_load_layout(void);
void xenon_pad_save_layout(void);

// The Settings pages (wh_menu360.c), and the version on the main menu.
void wh360_buttonlayoutmenu(void);
void wh360_controlsmenu(void);
void wh360_drawversion(void);
// The main menu's Help pages on the controls (wh360_helppages of them, before the credits), drawn without nextpage().
int wh360_helppages(void);
void wh360_helppage(int page);

#endif
