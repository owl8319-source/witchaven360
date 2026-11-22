// Witchaven 360: what the platform layer (xenonlayer.c) and the controller code (wh_pad.c) give each other.

#ifndef XENONLAYER_H
#define XENONLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

// From xenonlayer.c.

// A key going down (down = 1) or up, as a keyboard's would: keystatus[] and the key FIFO. code is the game's scan code.
void xenon_keyevent(int code, int down);
// A character for the game's text entry.
void xenon_charevent(int ch);

// The first controller, through SDL3: whether there is one, a button (an SDL_GamepadButton) held, an axis (an
// SDL_GamepadAxis, -32768 to 32767, the triggers 0 to 32767), and shaking it (0 to 65535 a motor).
int xenon_pad_connected(void);
int xenon_pad_button(int button);
int xenon_pad_axis(int axis);
void xenon_pad_rumble(int low, int high, int ms);

// Whether the Guide or one of the console's dialogs is up, as of the last handleevents().
int xenon_system_ui_open(void);

// From wh_pad.c.

// Once a handleevents(): turns the controller into keys and stick movement.
void xenon_pad_update(void);
// Lets every key the controller is holding down go (releaseallbuttons()).
void xenon_releaseallkeys(void);
// Once a game tic, from getinput() (whinp.c): the game, not a menu, is reading the controls.
void xenon_pad_gametic(void);

#ifdef __cplusplus
}
#endif

#endif
