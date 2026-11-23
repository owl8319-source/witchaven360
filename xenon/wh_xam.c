// Witchaven 360: the console's own on-screen keyboard, for naming saves (whmenu.c's savegametext), through xsdk's
// XAM calls (xsdk/xam.h), as Heretic 360's xenon/i_xam.c. The keyboard is started in one call and asked once a frame
// whether it's done, as xsdk's docs/GUIDE.md asks ("Poll once a frame, with a timeout of 0"): XAM's dialogs only
// show while the title keeps presenting frames, which the save screen does while it waits.

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xsdk/xam.h"

#include "wh_xam.h"

#define KEYBOARD_CHARS 32

// "Save Game" and "Name this save.", as UTF-16.
static const uint16_t keyboard_title[] = { 'S', 'a', 'v', 'e', ' ', 'G', 'a', 'm', 'e', 0 };
static const uint16_t keyboard_description[] = {
    'N', 'a', 'm', 'e', ' ', 't', 'h', 'i', 's', ' ', 's', 'a', 'v', 'e', '.', 0
};

static XAM_OVERLAPPED overlapped;
static uint16_t default_wide[KEYBOARD_CHARS + 1];
static uint16_t typed_wide[KEYBOARD_CHARS + 1];
static int pending = 0;

// The signed-in user's controller, or the first controller with nobody signed in.
static uint32_t KeyboardUser(void)
{
    uint32_t user;
    char gamertag[XSDK_XAM_GAMERTAG_BYTES];

    if (xsdk_xam_signed_in_user(&user, gamertag) != ERROR_SUCCESS || user == XSDK_XAM_USER_NONE) {
        return 0;
    }

    return user;
}

int wh360_keyboard_start(const char *default_text, int max_chars)
{
    uint32_t returned;
    int i;

    if (pending) {
        return 0;
    }

    if (max_chars > KEYBOARD_CHARS) {
        max_chars = KEYBOARD_CHARS;
    }

    for (i = 0; i < max_chars && default_text[i] != '\0'; i++) {
        default_wide[i] = (unsigned char)default_text[i];
    }
    default_wide[i] = 0;
    typed_wide[0] = 0;

    xsdk_xam_prepare(&overlapped);
    returned = xsdk_xam_returned(&overlapped,
                                 xsdk_xam_keyboard(KeyboardUser(), keyboard_title, keyboard_description, default_wide,
                                                   typed_wide, (uint32_t)max_chars + 1, &overlapped));

    if (returned != ERROR_IO_PENDING) {
        printf("witchaven360: the keyboard didn't open: 0x%08X\n", (unsigned)returned);
        return 0;
    }

    pending = 1;
    return 1;
}

int wh360_keyboard_poll(char *text, int text_bytes)
{
    uint32_t result;
    int i, n = 0;

    if (!pending) {
        return -1;
    }

    result = xsdk_xam_wait(&overlapped, 0, NULL);
    if (result == ERROR_IO_INCOMPLETE) {
        return 0;
    }

    pending = 0;

    if (result != ERROR_SUCCESS) {
        // 0x4C7, ERROR_CANCELLED: B on the keyboard.
        if (result == 0x4C7) printf("witchaven360: the keyboard was cancelled; nothing saved\n");
        else printf("witchaven360: the keyboard closed with 0x%08X\n", (unsigned)result);
        return -1;
    }

    // Witchaven's letters (whmenu.c's fancy[]): a to z, digits, ! ? - : and spaces; capitals made small, anything
    // else left out.
    for (i = 0; typed_wide[i] != 0 && n < text_bytes - 1; i++) {
        const uint16_t c = typed_wide[i];

        if (c < 0x80 && (isalnum((int)c) || strchr("!?-: ", (int)c))) {
            text[n++] = (char)tolower((int)c);
        }
    }
    text[n] = '\0';

    return 1;
}
