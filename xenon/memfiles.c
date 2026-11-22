// Witchaven 360: the game's data files read whole into memory the first time they're opened, so the game never waits
// on the hard drive while it plays. As Heretic 360 found, a read after the drive has gone idle can take seconds; the
// Build engine reads a tile from the .art files, and a sound from joesnd, the first time each is needed, so without
// this a door's first sound or a new room's textures could stall the game a second or two.
//
// The engine opens files with open() and reads them with read(), lseek() and fstat() (jfbuild's cache1d.c and
// compat.c). The link wraps those calls (CMakeLists.txt: -Wl,--wrap=open and so on): opening one of the data files below
// for reading returns a handle of this file's own, served from memory; everything else (settings, saves, the log) goes
// to the real calls as before. The tiles, sounds and music are all opened as the game starts, so they're all read
// then, about 38 MB, once.

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <SDL3/SDL.h>

int __real_open(const char *path, int flags, ...);
ssize_t __real_read(int fd, void *buf, size_t count);
off_t __real_lseek(int fd, off_t offset, int whence);
int __real_close(int fd);
int __real_fstat(int fd, struct stat *st);

int __wrap_open(const char *path, int flags, ...);
ssize_t __wrap_read(int fd, void *buf, size_t count);
off_t __wrap_lseek(int fd, off_t offset, int whence);
int __wrap_close(int fd);
int __wrap_fstat(int fd, struct stat *st);

#define MAX_FILES 64
#define MAX_HANDLES 64
// Handles from here up are memory files; the console's own stay far below.
#define HANDLE_BASE 0x4000
#define NAME_BYTES 32

typedef struct {
    char name[NAME_BYTES];  // lower case, without its folder
    unsigned char *data;
    size_t size;
} memfile_t;

typedef struct {
    memfile_t *file;  // NULL when the handle is free
    size_t pos;
} memhandle_t;

static memfile_t files[MAX_FILES];
static int file_count;
static memhandle_t handles[MAX_HANDLES];
static SDL_Mutex *lock;

// The file's name without its folder, in lower case; false if it doesn't fit.
static int BaseName(const char *path, char *name)
{
    const char *base = path;
    const char *p;
    size_t i;

    for (p = path; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ':') base = p + 1;
    }

    for (i = 0; base[i]; i++) {
        if (i >= NAME_BYTES - 1) return 0;
        name[i] = (char)tolower((unsigned char)base[i]);
    }
    name[i] = 0;
    return i > 0;
}

static int EndsWith(const char *name, const char *ending)
{
    const size_t n = strlen(name), e = strlen(ending);
    return n >= e && !strcmp(name + n - e, ending);
}

// The data files: the tiles, the maps, the palette and shading tables, the sounds, the music and films. Not the
// settings or the saves (svgn*.dat), which change.
static int IsGameData(const char *name)
{
    return (!strncmp(name, "tiles", 5) && EndsWith(name, ".art")) ||
           (!strncmp(name, "level", 5) && EndsWith(name, ".map")) ||
           !strcmp(name, "palette.dat") || !strcmp(name, "lookup.dat") || !strcmp(name, "tables.dat") ||
           !strcmp(name, "joesnd") || !strcmp(name, "songs") || EndsWith(name, ".smk");
}

// The file in memory, read from the drive the first time; NULL if it can't be read (the real open then says why).
static memfile_t *LoadFile(const char *path, const char *name)
{
    int i, fd;
    off_t size;
    size_t done = 0;
    unsigned char *data;
    const Uint64 started = SDL_GetTicks();

    for (i = 0; i < file_count; i++) {
        if (!strcmp(files[i].name, name)) return &files[i];
    }

    if (file_count >= MAX_FILES) return NULL;

    fd = __real_open(path, O_RDONLY);
    if (fd < 0) return NULL;

    size = __real_lseek(fd, 0, SEEK_END);
    if (size < 0 || __real_lseek(fd, 0, SEEK_SET) != 0) {
        __real_close(fd);
        return NULL;
    }

    data = (unsigned char *)malloc(size > 0 ? (size_t)size : 1);
    if (data == NULL) {
        printf("memfiles: no memory for %s (%ld bytes); reading it from the drive\n", name, (long)size);
        __real_close(fd);
        return NULL;
    }

    while (done < (size_t)size) {
        const ssize_t got = __real_read(fd, data + done, (size_t)size - done);
        if (got <= 0) {
            printf("memfiles: reading %s stopped at %lu of %ld bytes\n", name, (unsigned long)done, (long)size);
            free(data);
            __real_close(fd);
            return NULL;
        }
        done += (size_t)got;
    }

    __real_close(fd);

    strcpy(files[file_count].name, name);
    files[file_count].data = data;
    files[file_count].size = (size_t)size;
    printf("memfiles: %s, %ld bytes, in %u ms\n", name, (long)size, (unsigned)(SDL_GetTicks() - started));
    return &files[file_count++];
}

static memhandle_t *Handle(int fd)
{
    const int i = fd - HANDLE_BASE;
    if (i < 0 || i >= MAX_HANDLES || handles[i].file == NULL) return NULL;
    return &handles[i];
}

int __wrap_open(const char *path, int flags, ...)
{
    char name[NAME_BYTES];
    int mode = 0;

    if (flags & O_CREAT) {
        va_list va;
        va_start(va, flags);
        mode = va_arg(va, int);
        va_end(va);
    }

    if ((flags & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDONLY && path && BaseName(path, name) && IsGameData(name)) {
        memfile_t *file;
        int i;

        if (lock == NULL) lock = SDL_CreateMutex();
        if (lock) SDL_LockMutex(lock);

        file = LoadFile(path, name);
        if (file) {
            for (i = 0; i < MAX_HANDLES; i++) {
                if (handles[i].file == NULL) {
                    handles[i].file = file;
                    handles[i].pos = 0;
                    if (lock) SDL_UnlockMutex(lock);
                    return HANDLE_BASE + i;
                }
            }
            printf("memfiles: out of handles opening %s; reading it from the drive\n", name);
        }

        if (lock) SDL_UnlockMutex(lock);
    }

    return __real_open(path, flags, mode);
}

ssize_t __wrap_read(int fd, void *buf, size_t count)
{
    memhandle_t *h = Handle(fd);
    size_t left;

    if (h == NULL) return __real_read(fd, buf, count);

    left = h->pos < h->file->size ? h->file->size - h->pos : 0;
    if (count > left) count = left;
    memcpy(buf, h->file->data + h->pos, count);
    h->pos += count;
    return (ssize_t)count;
}

off_t __wrap_lseek(int fd, off_t offset, int whence)
{
    memhandle_t *h = Handle(fd);
    off_t base;

    if (h == NULL) return __real_lseek(fd, offset, whence);

    switch (whence) {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = (off_t)h->pos; break;
        case SEEK_END: base = (off_t)h->file->size; break;
        default: errno = EINVAL; return -1;
    }

    if (base + offset < 0) {
        errno = EINVAL;
        return -1;
    }

    h->pos = (size_t)(base + offset);
    return (off_t)h->pos;
}

int __wrap_close(int fd)
{
    memhandle_t *h = Handle(fd);

    if (h == NULL) return __real_close(fd);

    // The file stays in memory for the next open.
    h->file = NULL;
    return 0;
}

int __wrap_fstat(int fd, struct stat *st)
{
    memhandle_t *h = Handle(fd);

    if (h == NULL) return __real_fstat(fd, st);

    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG | 0444;
    st->st_size = (off_t)h->file->size;
    return 0;
}
