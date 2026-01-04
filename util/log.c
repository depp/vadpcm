// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#define _POSIX_C_SOURCE 200112L
#ifdef _GNU_SOURCE
#error "Do not define _GNU_SOURCE"
#endif

#include "util/util.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

static const char *strip_file_prefix(const char *file) {
    const char *ref = __FILE__;
    size_t ref_len = strlen(ref);
    size_t file_len = strlen(file);
    enum { UTIL_LOG_LEN = 10 };
    if (ref_len > UTIL_LOG_LEN) {
        ref_len -= UTIL_LOG_LEN;
        if (file_len > ref_len && memcmp(ref, file, ref_len) == 0) {
            file += ref_len;
        }
    }
    return file;
}

static void log_msg(int level, const char *file, int line, bool has_errcode,
                    int errcode, const char *fmt, va_list ap) {
    fprintf(stderr, "\33[%sm%s\33[0m: %s:%d: ", LEVELS[level].color,
            LEVELS[level].name, strip_file_prefix(file), line);
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
