// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

// ============================================================================
// Logging
// ============================================================================

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

// ============================================================================
// Byte Order
// ============================================================================

// https://github.com/cpredef/predef/blob/master/Endianness.md

#define FOURCC_NATIVE(c1, c2, c3, c4)                                          \
    (((uint32_t)(c1) << 24) | ((uint32_t)(c2) << 16) | ((uint32_t)(c3) << 8) | \
     (uint32_t)(c4))
#define FOURCC_SWAPPED(c1, c2, c3, c4) FOURCC_NATIVE(c4, c3, c2, c1)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FOURCC_LE FOURCC_NATIVE
#define FOURCC_BE FOURCC_SWAPPED
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FOURCC_LE FOURCC_SWAPPED
#define FOURCC_BE FOURCC_NATIVE
#else
#error "Unknown byte order"
#endif
