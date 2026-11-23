// Witchaven 360: the console's own on-screen keyboard, for naming saves (wh_xam.c).

#ifndef WH_XAM_H
#define WH_XAM_H

// Opens the keyboard with default_text in it, for up to max_chars characters. 0 if it can't open: the console's UI is
// already up, or the call failed.
int wh360_keyboard_start(const char *default_text, int max_chars);

// Once a frame while the keyboard is open: 0 while it's up, 1 once a name was accepted (in text, up to text_bytes with
// its zero, in Witchaven's letters: small a to z, digits, ! ? - : and spaces), -1 once it was cancelled or failed.
int wh360_keyboard_poll(char *text, int text_bytes);

#endif
