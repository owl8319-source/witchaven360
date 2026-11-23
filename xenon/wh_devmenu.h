// Witchaven 360: the developer cheat menu (wh_devmenu.c).
#ifndef WH_DEVMENU_H
#define WH_DEVMENU_H

// Whether it's open; the game's clock stands still while it is (xenonlayer.c), as behind the Guide.
int wh360_devmenu_active(void);
// Opens it (wh_pad.c, on LB + RB + Back held), and runs it: up and down choose, select runs the row, close closes.
void wh360_devmenu_open(void);
void wh360_devmenu_input(int up, int down, int select, int close);
// Draws it over the game's frame, if open (whaven.c's drawscreen, before nextpage).
void wh360_devmenu_draw(void);
// God mode: the player takes no harm (whplr.c's healthpic, whani.c).
int wh360_godmode(void);

#endif
