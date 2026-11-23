// Witchaven 360: the Settings pages for the controller, opened from the settings menu (whmenu.c's configmenu), and
// the version on the main menu.
//
// Button layout: a row per button (wh_pad.h's order) and what it does in the game; left and right go through the
// actions, the last row puts them back as the game starts. Kept in controller.cfg (wh_pad.c) on the way out.
//
// Controls: the controller (art/360_controller.svg in Witchaven's palette, controls_art.h) over the title picture,
// darkened, and what each button does now.
//
// Help: the main menu's Help pages on the controls, with the controller's buttons.
//
// All are drawn as the settings menu draws itself: Witchaven's 640x480 screen in the middle of the display (svgaxoff,
// svgascale), the headings in the game's gold letters (THEFONT), the rows in the engine's 8x14 font.

#include "icorp.h"
#include "osd.h"

#include <stdio.h>
#include <string.h>

#include "wh_pad.h"
#include "controls_art.h"

extern void fancyfont(int x, int y, short tilenum, char *string, char pal);
extern void svgafullscreenpic(short pic1, short pic2);
extern void TEMPSND(void);

// The engine's palette: white, light blue (the row chosen), green (its value), gold, and near-black for shadows.
#define COLOUR_TEXT 29
#define COLOUR_CHOSEN 237
#define COLOUR_VALUE 125
#define COLOUR_GOLD 242
#define COLOUR_SHADOW 2
#define FONT_TALL 2
#define FONT_W 8

// How much darker the pages' background is than the title picture, of the palette's shades (0 as bright as the
// game), so the text reads over it: as the settings menu's (whmenu.c), and more behind the controller.
#define MENU_DARKNESS 16
#define CONTROLS_DARKNESS 22

// From the 640x480 screen to the display's pixels.
static int SX(int x)
{
    return (svgaxoff >> 16) + mulscale16(x, svgascale);
}

static int SY(int y)
{
    return mulscale16(y, svgascale);
}

// With a shadow, to read over the picture.
static void Text(int x, int y, int colour, const char *s)
{
    printext256(SX(x) + 1, SY(y) + 1, COLOUR_SHADOW, -1, s, FONT_TALL);
    printext256(SX(x), SY(y), colour, -1, s, FONT_TALL);
}

// In the display's pixels, centred on the 640x480 screen's x, with a shadow.
static void CentredText(int x, int y, int colour, const char *s)
{
    const int px = SX(x) - (int)strlen(s) * FONT_W / 2;

    printext256(px + 1, SY(y) + 1, COLOUR_SHADOW, -1, s, FONT_TALL);
    printext256(px, SY(y), colour, -1, s, FONT_TALL);
}

// A heading in the game's gold letters (THEFONT: a to z, digits, ! ? - : and space), centred.
static void Heading(int y, const char *s)
{
    char text[40];
    int i, width = 0;

    strncpy(text, s, sizeof(text) - 1);
    text[sizeof(text) - 1] = 0;

    for (i = 0; text[i]; i++) {
        const char *p = strchr("abcdefghijklmnopqrstuvwxyz0123456789!?-:", text[i]);
        width += (text[i] == ' ' || !p) ? 8 : tilesizx[THEFONT + (p - "abcdefghijklmnopqrstuvwxyz0123456789!?-:")] + 1;
    }

    fancyfont(320 - width / 2, y, THEFONT, text, 0);
}

static void Background(int shade)
{
    clearallviews(0);
    if (svga) {
        rotatesprite(svgaxoff, 0 << 15, svgascale, 0, SVGAMENU, shade, 0, 8 + 16 + 64, 0, 0, xdim - 1, ydim - 1);
        rotatesprite(svgaxoff, ydim << 15, svgascale, 0, SVGAMENU2, shade, 0, 8 + 16 + 64, 0, 0, xdim - 1, ydim - 1);
    }
    rotatesprite(svgaxoff, 0 << 15, svgascale, 0, TITLEPIC, shade, 0, svgastat, 0, 0, xdim - 1, ydim - 1);
}

// Leaves when B or Start (Escape) is pressed, or A (Enter) with 'enter_leaves'; waits for it to be let go.
static int Leave(int enter_leaves)
{
    if (keystatus[1]) {
        keystatus[1] = 0;
        return 1;
    }
    if (enter_leaves && (keystatus[0x1c] || keystatus[0x39])) {
        keystatus[0x1c] = keystatus[0x39] = 0;
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Button layout

#define LAYOUT_TOP 64
#define LAYOUT_ROW 18
#define LAYOUT_BUTTON_X 150
#define LAYOUT_ACTION_X 360

void wh360_buttonlayoutmenu(void)
{
    const int rows = PAD_BUTTONS + 1;  // and "Reset to defaults"
    int select = 0, delaytime = totalclock + 10, i, changed = 0;

    keystatus[0x1c] = keystatus[0x39] = 0;

    while (1) {
        handleevents();
        OSD_DispatchQueued();

        if (Leave(0)) break;

        if (totalclock >= delaytime) {
            if (keystatus[LUP] || keystatus[RUP]) {
                select = (select + rows - 1) % rows;
                delaytime = totalclock + 15;
            } else if (keystatus[LDN] || keystatus[RDN]) {
                select = (select + 1) % rows;
                delaytime = totalclock + 15;
            } else if (select < PAD_BUTTONS && (keystatus[LLEFT] || keystatus[RLEFT] || keystatus[LRIGHT] ||
                                                keystatus[RRIGHT])) {
                const int dir = (keystatus[LLEFT] || keystatus[RLEFT]) ? PA_COUNT - 1 : 1;

                xenon_pad_set_action(select, (xenon_pad_action(select) + dir) % PA_COUNT);
                changed = 1;
                TEMPSND();
                delaytime = totalclock + 20;
            } else if (select == PAD_BUTTONS && keystatus[0x1c]) {
                keystatus[0x1c] = keystatus[0x39] = 0;
                xenon_pad_reset_layout();
                changed = 1;
                TEMPSND();
                delaytime = totalclock + 20;
            }
        }

        Background(MENU_DARKNESS);
        Heading(18, "button layout");

        for (i = 0; i < rows; i++) {
            const int y = LAYOUT_TOP + i * LAYOUT_ROW + (i == PAD_BUTTONS ? LAYOUT_ROW / 2 : 0);

            if (i < PAD_BUTTONS) {
                Text(LAYOUT_BUTTON_X, y, i == select ? COLOUR_CHOSEN : COLOUR_TEXT, xenon_pad_button_name(i));
                Text(LAYOUT_ACTION_X, y, i == select ? COLOUR_VALUE : COLOUR_TEXT,
                     xenon_pad_action_name(xenon_pad_action(i)));
            } else {
                Text(LAYOUT_BUTTON_X, y, i == select ? COLOUR_CHOSEN : COLOUR_TEXT, "Reset to defaults");
            }
        }

        CentredText(320, LAYOUT_TOP + rows * LAYOUT_ROW + 22, COLOUR_GOLD,
                    "Left and right change a button   B goes back");
        CentredText(320, LAYOUT_TOP + rows * LAYOUT_ROW + 40, COLOUR_GOLD, "Start always opens the menu");

        nextpage();
    }

    if (changed) xenon_pad_save_layout();
}

// ---------------------------------------------------------------------------------------------------------------------
// Controls

#define ART_TOP 46
#define LIST_TOP 232
#define LIST_ROW 17
#define LEFT_BUTTON_X 36
#define LEFT_ACTION_X 124
#define RIGHT_BUTTON_X 336
#define RIGHT_ACTION_X 452

// The controller, each of its pixels a block of the display's (more than one at 720 lines), clear where it's clear.
static void DrawController(void)
{
    const unsigned char *in = controls_art;
    const unsigned char *const end = controls_art + sizeof(controls_art);
    const int left = 320 - CONTROLS_ART_W / 2;
    int i = 0;

    while (in < end && i < CONTROLS_ART_W * CONTROLS_ART_H) {
        const int header = *in++;
        const int count = (header & 0x7f) + 1;
        int k;

        for (k = 0; k < count && i < CONTROLS_ART_W * CONTROLS_ART_H; k++, i++) {
            const int ax = left + i % CONTROLS_ART_W, ay = ART_TOP + i / CONTROLS_ART_W;
            int x0, x1, y0, y1, x, y;
            unsigned char colour;

            if (header & 0x80) continue;
            colour = *in++;

            x0 = SX(ax); x1 = SX(ax + 1); y0 = SY(ay); y1 = SY(ay + 1);
            if (x0 < 0) x0 = 0;
            if (x1 > xdim) x1 = xdim;
            if (y1 > ydim) y1 = ydim;
            for (y = y0; y < y1; y++) {
                unsigned char *row = (unsigned char *)(frameplace + ylookup[y]);
                for (x = x0; x < x1; x++) row[x] = colour;
            }
        }
    }
}

static void Row(int x_button, int x_action, int row, const char *button, const char *action)
{
    const int y = LIST_TOP + row * LIST_ROW;

    Text(x_button, y, COLOUR_GOLD, button);
    Text(x_action, y, COLOUR_TEXT, action);
}

void wh360_controlsmenu(void)
{
    // The list's two columns: the sticks and Start, then the layout's buttons, short names.
    static const char *const short_names[PAD_BUTTONS] = {
        "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "Back", "L click", "R click", "D-up", "D-down", "D-left",
        "D-right",
    };
    int i;

    keystatus[0x1c] = keystatus[0x39] = 0;

    while (1) {
        handleevents();
        OSD_DispatchQueued();

        if (Leave(1)) break;

        Background(CONTROLS_DARKNESS);
        Heading(12, "controls");
        DrawController();

        Row(LEFT_BUTTON_X, LEFT_ACTION_X, 0, "L stick", "Move");
        for (i = 0; i < 8; i++) {
            Row(LEFT_BUTTON_X, LEFT_ACTION_X, i + 1, short_names[i], xenon_pad_action_name(xenon_pad_action(i)));
        }
        Row(RIGHT_BUTTON_X, RIGHT_ACTION_X, 0, "R stick", "Turn / look");
        for (i = 8; i < PAD_BUTTONS; i++) {
            Row(RIGHT_BUTTON_X, RIGHT_ACTION_X, i - 7, short_names[i], xenon_pad_action_name(xenon_pad_action(i)));
        }
        Row(RIGHT_BUTTON_X, RIGHT_ACTION_X, PAD_BUTTONS - 7, "Start", "Menu");

        CentredText(320, LIST_TOP + 9 * LIST_ROW + 12, COLOUR_GOLD, "Change them in Settings - Button layout");

        nextpage();
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// The version, bottom right of the display, in the main menu's white with a shadow.

// ---------------------------------------------------------------------------------------------------------------------
// Help

// The main menu's Help pages on weapons, spells, potions, movement and flying are pictures, with the PC's keys drawn
// into them; these are drawn in their place, with the controller's buttons as Settings > Button layout has them. (The
// credits after them are the game's own.)

#define HELP_TOP 66
#define HELP_ROW 22
#define HELP_KEY_X 300
#define HELP_WHAT_X 320

// The buttons doing 'action', "A / B" if more than one, or "(no button)".
static const char *ButtonsFor(int action, char *buf, int size)
{
    int b, n = 0;

    buf[0] = 0;
    for (b = 0; b < PAD_BUTTONS; b++) {
        if (xenon_pad_action(b) != action) continue;
        if (n++) strncat(buf, " / ", size - strlen(buf) - 1);
        strncat(buf, xenon_pad_button_name(b), size - strlen(buf) - 1);
    }
    if (!n) snprintf(buf, size, "(no button)");
    return buf;
}

// "LB / RB" for a previous and a next.
static const char *Pair(int prev, int next, char *buf, int size)
{
    char a[48], b[48];

    snprintf(buf, size, "%s / %s", ButtonsFor(prev, a, sizeof(a)), ButtonsFor(next, b, sizeof(b)));
    return buf;
}

// A row: the button, right-aligned to the middle in gold, and what it does after it.
static void HelpRow(int row, const char *key, const char *what)
{
    const int y = HELP_TOP + row * HELP_ROW;

    Text(HELP_KEY_X - (int)strlen(key) * FONT_W * 65536 / svgascale, y, COLOUR_GOLD, key);
    Text(HELP_WHAT_X, y, COLOUR_TEXT, what);
}

// A list in two columns, under the rows.
static void HelpList(int row, const char *const *items, int count)
{
    const int half = (count + 1) / 2;
    int i;

    for (i = 0; i < count; i++)
        Text(i < half ? 150 : 350, HELP_TOP + (row + i % half) * HELP_ROW, COLOUR_TEXT, items[i]);
}

int wh360_helppages(void)
{
    return 5;
}

void wh360_helppage(int page)
{
    static const char *const weapons[] = {
        "Fists", "Dagger", "Short sword", "Morning star", "Broad sword",
        "Battle axe", "Bow", "Throwing axe", "Magical sword", "Halberd",
    };
    static const char *const spells[] = {
        "Scare", "Night vision", "Freeze", "Magic arrows", "Open doors", "Fly", "Fireball", "Nuke!",
    };
    static const char *const potions[] = {
        "Blue: health", "Green: strength", "Orange: cure poison", "Red: resist fire", "Brown: invisibility",
    };
    char k[100];

    Background(MENU_DARKNESS);

    switch (page) {
    case 0:
        Heading(18, "weapons");
        HelpRow(0, ButtonsFor(PA_ATTACK, k, sizeof(k)), "Use the weapon");
        HelpRow(1, Pair(PA_PREV_WEAPON, PA_NEXT_WEAPON, k, sizeof(k)), "Previous / next weapon");
        CentredText(320, HELP_TOP + 3 * HELP_ROW, COLOUR_GOLD, "The weapons, as you find them:");
        HelpList(4, weapons, 10);
        break;
    case 1:
        Heading(18, "spells");
        HelpRow(0, ButtonsFor(PA_CAST, k, sizeof(k)), "Cast the spell chosen");
        HelpRow(1, Pair(PA_PREV_SPELL, PA_NEXT_SPELL, k, sizeof(k)), "Previous / next spell");
        CentredText(320, HELP_TOP + 3 * HELP_ROW, COLOUR_GOLD, "The spells, as you find them:");
        HelpList(4, spells, 8);
        break;
    case 2:
        Heading(18, "potions");
        HelpRow(0, ButtonsFor(PA_DRINK, k, sizeof(k)), "Drink the potion chosen");
        HelpRow(1, Pair(PA_PREV_POTION, PA_NEXT_POTION, k, sizeof(k)), "Previous / next potion");
        CentredText(320, HELP_TOP + 3 * HELP_ROW, COLOUR_GOLD, "The potions:");
        HelpList(4, potions, 5);
        break;
    case 3:
        Heading(18, "movement");
        HelpRow(0, "Left stick", "Move and strafe");
        HelpRow(1, "Right stick", xenon_pad_invert_look() ? "Turn and look (inverted)" : "Turn and look");
        HelpRow(2, ButtonsFor(PA_RUN, k, sizeof(k)), "Run, while held");
        HelpRow(3, ButtonsFor(PA_USE, k, sizeof(k)), "Open doors, use switches");
        HelpRow(4, ButtonsFor(PA_JUMP, k, sizeof(k)), "Jump");
        HelpRow(5, ButtonsFor(PA_CROUCH, k, sizeof(k)), "Crouch");
        HelpRow(6, ButtonsFor(PA_CENTRE, k, sizeof(k)), "Centre the view");
        HelpRow(7, ButtonsFor(PA_MAP, k, sizeof(k)), "Map");
        HelpRow(8, "D-pad up / down", "Zoom the map in / out");
        HelpRow(9, "Start", "Menu");
        break;
    case 4:
        Heading(18, "flying");
        {
            char pair[100], cast[48], line[200];
            snprintf(line, sizeof(line), "Choose Fly (%s), cast it (%s), then:",
                     Pair(PA_PREV_SPELL, PA_NEXT_SPELL, pair, sizeof(pair)), ButtonsFor(PA_CAST, cast, sizeof(cast)));
            CentredText(320, HELP_TOP, COLOUR_GOLD, line);
        }
        HelpRow(2, ButtonsFor(PA_JUMP, k, sizeof(k)), "Fly up");
        HelpRow(3, ButtonsFor(PA_CROUCH, k, sizeof(k)), "Fly down, and land on the ground");
        {
            int b, bound = 0;
            for (b = 0; b < PAD_BUTTONS; b++) bound |= xenon_pad_action(b) == PA_END_FLYING;
            if (bound) HelpRow(4, ButtonsFor(PA_END_FLYING, k, sizeof(k)), "End flying");
        }
        CentredText(320, HELP_TOP + 6 * HELP_ROW, COLOUR_TEXT, "Flying also ends when the spell wears off.");
        CentredText(320, HELP_TOP + 7 * HELP_ROW, COLOUR_TEXT, "End flying can be given a button in Settings.");
        break;
    }

    CentredText(320, 440, COLOUR_GOLD, "Left and right turn the pages - B goes back");
}

void wh360_drawversion(void)
{
    const char *version = "v" WITCHAVEN360_VERSION;
    const int x = xdim - (int)strlen(version) * FONT_W - 12, y = ydim - 18;

    printext256(x + 2, y + 2, COLOUR_SHADOW, -1, version, FONT_TALL);
    printext256(x, y, COLOUR_TEXT, -1, version, FONT_TALL);
}
