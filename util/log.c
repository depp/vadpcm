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

static void log_msg(const char *file, int line, bool has_errcode, int errcode,
                    const char *fmt, va_list ap) {
    fprintf(stderr, "\33[1;31mError\33[0m: %s:%d: ", file, line);
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
    log_msg(file, line, false, 0, fmt, ap);
    va_end(ap);
}

void log_error_errno(const char *file, int line, int errcode, const char *fmt,
                     ...) {
    va_list ap;
    va_start(ap, fmt);
    log_msg(file, line, true, errcode, fmt, ap);
    va_end(ap);
}
