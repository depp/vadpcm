// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.

// Show an error message.
void log_error(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

// Show an error message, with an error code from errno appended.
void log_error_errno(const char *file, int line, int errcode, const char *fmt,
                     ...) __attribute__((format(printf, 4, 5)));

// Show an informational message.
void log_info(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

// Show a debugging message.
void log_debug(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define LOG_ERROR(...) log_error(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR_ERRNO(errcode, ...) \
    log_error_errno(__FILE__, __LINE__, errcode, __VA_ARGS__)
#define LOG_INFO(...) log_info(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_debug(__FILE__, __LINE__, __VA_ARGS__)
