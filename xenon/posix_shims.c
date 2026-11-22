// Witchaven 360: two POSIX calls newlib declares but xsdk doesn't provide.
//
// fstatat: jfbuild asks it for a file's size and type while listing a folder (compat.c's Breaddir, kplib.c's
// kzfindfile). Here it says it can't, so a listing still names the files, with no sizes; the game doesn't list folders
// as it plays.
//
// times: newlib's clock() is built on it, and the network code (whnet.c) calls clock(). The time since the game
// started, in clock()'s milliseconds (CLOCKS_PER_SEC is 1000 in newlib), from SDL's timer.

#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>

#include <SDL3/SDL.h>

int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
    (void)dirfd; (void)pathname; (void)statbuf; (void)flags;
    errno = ENOSYS;
    return -1;
}

clock_t times(struct tms *buf)
{
    const clock_t ms = (clock_t)SDL_GetTicks();

    if (buf) {
        buf->tms_utime = ms;
        buf->tms_stime = 0;
        buf->tms_cutime = 0;
        buf->tms_cstime = 0;
    }

    return ms;
}
