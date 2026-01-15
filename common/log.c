// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#define _POSIX_C_SOURCE 200112L
#ifdef _GNU_SOURCE
#error "Do not define _GNU_SOURCE"
#endif
#define _DARWIN_C_SOURCE 1

#include "common/util.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define LOCK_FILE(x) (void)0
#define UNLOCK_FILE(x) (void)0
#else
#define LOCK_FILE flockfile
#define UNLOCK_FILE funlockfile
#endif

#if _MSC_VER
#pragma warning(disable : 6504)
#endif

log_level g_log_level = LEVEL_INFO;

struct log_context {
    const char *operation;
    const char *path;
};

struct log_context g_log_context;

struct log_level {
    char color[5];
    char name[6];
};

static const struct log_level LEVELS[] = {
    [LEVEL_ERROR] = {.color = "1;31", .name = "Error"},
    [LEVEL_INFO] = {.color = "32", .name = "Info"},
    [LEVEL_DEBUG] = {.color = "35", .name = "Debug"},
};

static void log_msg(log_level level, const char *file, int line,
                    bool has_errcode, int errcode, const char *fmt,
                    va_list ap) {
    if (level > g_log_level || level < 0) {
        return;
    }
    LOCK_FILE(stderr);
#if _WIN32
    // TODO: Color on Windows
    fprintf(stderr, "%s: ", LEVELS[level].name);
#else
    fprintf(stderr, "\33[%sm%s\33[0m: ", LEVELS[level].color,
            LEVELS[level].name);
#endif
    if (g_log_level >= LEVEL_DEBUG) {
        fprintf(stderr, "%s:%d: ", file, line);
    }
    struct log_context ctx = g_log_context;
    if (ctx.operation != NULL) {
        fputc(' ', stderr);
        fputs(ctx.operation, stderr);
        fputc(' ', stderr);
        fputs(ctx.path, stderr);
        fputs(": ", stderr);
    }
    vfprintf(stderr, fmt, ap);
    if (has_errcode) {
        fputs(": ", stderr);
#if _WIN32
        DWORD flags =
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        DWORD lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        char buf[1024];
        DWORD result = FormatMessageA(flags, NULL, errcode, lang_id, buf,
                                      sizeof(buf), NULL);
        if (result != 0) {
            fwrite(buf, 1, result, stderr);
        } else if (GetLastError() == ERROR_MORE_DATA) {
            char *ptr;
            flags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
            result = FormatMessageA(flags, NULL, errcode, lang_id, (char *)&ptr,
                                    0, NULL);
            if (result == 0) {
                abort();
            }
            fwrite(ptr, 1, result, stderr);
            LocalFree(ptr);
        } else {
            fprintf(stderr, "error code = %lu", errcode);
        }
#else
        char buf[1024];
        int r = strerror_r(errcode, buf, sizeof(buf));
        if (r == 0) {
            fputs(buf, stderr);
        } else {
            fprintf(stderr, "errno = %d", errcode);
        }
#endif
    }
    fputc('\n', stderr);
    UNLOCK_FILE(stderr);
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

void log_context(const char *operation, const char *path) {
    g_log_context = (struct log_context){.operation = operation, .path = path};
}

void log_context_clear(void) {
    g_log_context = (struct log_context){.operation = NULL, .path = NULL};
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
