// Witchaven 360: the developer cheat menu, as Heretic 360 has one.
//
// Hold LB + RB + Back for about a second in a level (wh_pad.c) and it opens over the game, which waits behind it, its
// clock standing still (xenonlayer.c). It has these rows, and they work on every difficulty:
//
//   God mode                         on and off; new: Witchaven has no such cheat
//   All weapons                      as typing scooter
//   All potions                      as mommy
//   All spells, keys and armour      as wango (and full health, level 7)
//   Kill every monster               the nuke spell's effect, on every monster in the level
//   Frame-rate counter               on and off, jfwhaven's showfps
//
// The typed cheats run the game's own code for them (whinp.c's checkcheat), not a copy. The menu isn't in the game's
// menus or settings.
//
// D-pad or left stick: choose. A: select. B: close.

#include "icorp.h"
#include "baselayer.h"

#include <stdio.h>
#include <string.h>

#include "wh_devmenu.h"

extern char displaybuf[];
extern int displaytime;
extern void nukespell(short j);
extern int wh360_showfps(int toggle);

enum { ROW_GOD, ROW_WEAPONS, ROW_POTIONS, ROW_SPELLS, ROW_MASSACRE, ROW_FPS, ROWS };

static const char *const row_names[ROWS] = {
    "God mode",
    "All weapons",
    "All potions",
    "All spells, keys and armour",
    "Kill every monster",
    "Frame-rate counter",
};

static int active = 0;
static int cursor = 0;
static int godmode = 0;

// The menu's box and rows, on a 640x480 screen in the middle of the display.
#define BOX_LEFT 130
#define BOX_RIGHT 510
#define BOX_TOP 120
#define BOX_BOTTOM 340
#define TITLE_Y 134
#define FIRST_ROW_Y 170
#define ROW_HEIGHT 22
#define ROW_X 170
#define VALUE_RIGHT 490
#define CURSOR_X 150
#define HINT_Y 312
// How dark the box behind it is, of the palette's shades (0 as bright as the game).
#define BOX_DARKNESS 24

// The engine's palette: white, light blue (the row chosen), green (a value), gold, near-black for shadows.
#define COLOUR_TEXT 29
#define COLOUR_CHOSEN 237
#define COLOUR_VALUE 125
#define COLOUR_GOLD 242
#define COLOUR_SHADOW 2
#define FONT_TALL 2
#define FONT_W 8

int wh360_devmenu_active(void)
{
    return active;
}

int wh360_godmode(void)
{
    return godmode;
}

void wh360_devmenu_open(void)
{
    active = 1;
    SND_Sound(S_PICKUPAXE);
}

static void Message(const char *text)
{
    strcpy(displaybuf, text);
    displaytime = 360;
}

// A typed cheat, run as whinp.c runs it once typed.
static void Typed(const char *cheat)
{
    strcpy(displaybuf, cheat);
    checkcheat();
}

static void KillEveryMonster(void)
{
    int i;

    // As the nuke spell does to the monsters it reaches (whobj.c's nukespell); anything else, it leaves.
    for (i = 0; i < MAXSPRITES; i++) {
        if (sprite[i].statnum >= MAXSTATUS) continue;
        nukespell((short)i);
    }
}

static void Select(void)
{
    switch (cursor) {
    case ROW_GOD:
        godmode = !godmode;
        Message(godmode ? "god mode on" : "god mode off");
        break;
    case ROW_WEAPONS:
        Typed("SCOOTER");
        Message("all weapons");
        break;
    case ROW_POTIONS:
        Typed("MOMMY");
        Message("all potions");
        break;
    case ROW_SPELLS:
        Typed("WANGO");
        Message("all spells, keys and armour");
        break;
    case ROW_MASSACRE:
        KillEveryMonster();
        Message("every monster killed");
        break;
    case ROW_FPS:
        wh360_showfps(1);
        break;
    }
    SND_Sound(S_PICKUPAXE);
}

void wh360_devmenu_input(int up, int down, int select, int close)
{
    if (!active) return;
    if (close) {
        active = 0;
        SND_Sound(S_PICKUPAXE);
        return;
    }
    if (up) cursor = (cursor + ROWS - 1) % ROWS;
    if (down) cursor = (cursor + 1) % ROWS;
    if (select) Select();
}

// From the 640x480 screen to the display's pixels.
static int SX(int x)
{
    return (xdim - 640 * ydim / 480) / 2 + x * ydim / 480;
}

static int SY(int y)
{
    return y * ydim / 480;
}

static void Text(int x, int y, int colour, const char *s)
{
    printext256(x + 1, y + 1, COLOUR_SHADOW, -1, s, FONT_TALL);
    printext256(x, y, colour, -1, s, FONT_TALL);
}

// Darkens the box through the palette's shades, as the game darkens a dim room.
static void DarkenBox(void)
{
    const unsigned char *shade = palookup[0] + (BOX_DARKNESS << 8);
    int left = SX(BOX_LEFT), right = SX(BOX_RIGHT), top = SY(BOX_TOP), bottom = SY(BOX_BOTTOM), x, y;

    if (!shade || !frameplace) return;
    if (left < 0) left = 0;
    if (right > xdim) right = xdim;
    if (bottom > ydim) bottom = ydim;
    for (y = top; y < bottom; y++) {
        unsigned char *row = (unsigned char *)frameplace + y * bytesperline;
        for (x = left; x < right; x++) row[x] = shade[row[x]];
    }
}

void wh360_devmenu_draw(void)
{
    static const char *const title = "CHEATS";
    static const char *const hint = "A select   B close";
    int i;

    if (!active) return;

    DarkenBox();
    Text(SX(320) - (int)strlen(title) * FONT_W / 2, SY(TITLE_Y), COLOUR_GOLD, title);
    for (i = 0; i < ROWS; i++) {
        const int y = SY(FIRST_ROW_Y + i * ROW_HEIGHT);
        const char *value = i == ROW_GOD ? (godmode ? "On" : "Off") : i == ROW_FPS ? (wh360_showfps(0) ? "On" : "Off")
                                                                                     : NULL;
        Text(SX(ROW_X), y, i == cursor ? COLOUR_CHOSEN : COLOUR_TEXT, row_names[i]);
        if (value) Text(SX(VALUE_RIGHT) - (int)strlen(value) * FONT_W, y, COLOUR_VALUE, value);
    }
    // The cursor blinks, as the game's menus blink theirs.
    if ((getticks() / 250) & 1) Text(SX(CURSOR_X), SY(FIRST_ROW_Y + cursor * ROW_HEIGHT), COLOUR_GOLD, ">");
    Text(SX(320) - (int)strlen(hint) * FONT_W / 2, SY(HINT_Y), COLOUR_GOLD, hint);
}
