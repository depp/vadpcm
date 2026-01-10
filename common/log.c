// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#define _POSIX_C_SOURCE 200112L
#ifdef _GNU_SOURCE
#error "Do not define _GNU_SOURCE"
#endif

#include "common/util.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // IWYU pragma: keep (false positive?)

enum {
    LEVEL_ERROR,
    LEVEL_INFO,
    LEVEL_DEBUG,
};

struct log_level {
    char color[5];
    char name[6];
};

static const struct log_level LEVELS[] = {
    [LEVEL_ERROR] = {.color = "1;31", .name = "Error"},
    [LEVEL_INFO] = {.color = "32", .name = "Info"},
    [LEVEL_DEBUG] = {.color = "35", .name = "Debug"},
};

static void log_msg(int level, const char *file, int line, bool has_errcode,
                    int errcode, const char *fmt, va_list ap) {
    flockfile(stderr);
    fprintf(stderr, "\33[%sm%s\33[0m: %s:%d: ", LEVELS[level].color,
            LEVELS[level].name, file, line);
    vfprintf(stderr, fmt, ap);
    if (has_errcode) {
        fputs(": ", stderr);
        char buf[1024];
        int r = strerror_r(errcode, buf, sizeof(buf));
        if (r == 0) {
            fputs(buf, stderr);
        } else {
            fprintf(stderr, "errno = %d", errcode);
        }
    }
    fputc('\n', stderr);
    funlockfile(stderr);
}

void log_error(const char *file, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_msg(LEVEL_ERROR, file, line, false, 0, fmt, ap);
    va_end(ap);
}

void log_error_errno(const char *file, int line, int errcode, const char *fmt,
                     ...) {
    va_list ap;
    va_start(ap, fmt);
    log_msg(LEVEL_ERROR, file, line, true, errcode, fmt, ap);
    va_end(ap);
}

void log_info(const char *file, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_msg(LEVEL_INFO, file, line, false, 0, fmt, ap);
    va_end(ap);
}

void log_debug(const char *file, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_msg(LEVEL_DEBUG, file, line, false, 0, fmt, ap);
    va_end(ap);
}

static char hexdigit(int x) {
    return x < 10 ? x + '0' : x - 10 + 'a';
}

void format_fourcc(char *buf, uint32_t fourcc) {
    *buf++ = '\'';
    for (int i = 0; i < 4; i++) {
        int c = (fourcc >> (24 - (i * 8))) & 0xff;
        if (32 <= c && c <= 126) {
            if (c == '\'' || c == '\'') {
                *buf++ = '\\';
            }
            *buf++ = c;
        } else {
            buf[0] = '\\';
            buf[1] = 'x';
            buf[2] = hexdigit(c >> 4);
            buf[3] = hexdigit(c & 15);
            buf += 4;
        }
    }
    *buf++ = '\'';
    *buf++ = '\0';
}
