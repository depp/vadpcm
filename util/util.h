// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include <stddef.h>
#include <stdint.h>

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
// Memory Allocation
// ============================================================================

void *xmalloc(const char *file, int line, size_t nmemb, size_t size)
    __attribute__((malloc, alloc_size(3, 4)));

#define XMALLOC(nmemb, size) xmalloc(__FILE__, __LINE__, nmemb, size)

// ============================================================================
// Serialization / Deserialization
// ============================================================================

inline uint16_t read16be(const uint8_t *ptr) {
    return ((uint16_t)ptr[0] << 8) | (uint16_t)ptr[1];
}

inline uint32_t read32be(const uint8_t *ptr) {
    return ((uint32_t)ptr[0] << 24) | //
           ((uint32_t)ptr[1] << 16) | //
           ((uint32_t)ptr[2] << 8) |  //
           (uint32_t)ptr[3];
}

inline uint64_t read64be(const uint8_t *ptr) {
    return ((uint64_t)ptr[0] << 56) | //
           ((uint64_t)ptr[1] << 48) | //
           ((uint64_t)ptr[2] << 40) | //
           ((uint64_t)ptr[3] << 32) | //
           ((uint64_t)ptr[4] << 24) | //
           ((uint64_t)ptr[5] << 16) | //
           ((uint64_t)ptr[6] << 8) |  //
           (uint64_t)ptr[7];
}

// ============================================================================
// FOURCC
// ============================================================================

#define FOURCC(c1, c2, c3, c4)                                                 \
    (((uint32_t)(c1) << 24) | ((uint32_t)(c2) << 16) | ((uint32_t)(c3) << 8) | \
     (uint32_t)(c4))

enum {
    // Buffer size for formatting a four character code.
    FOURCC_BUFSZ = 4 * 4 + 3,
};

// Format a four-character code for printing.
void format_fourcc(char *buf, uint32_t fourcc);

// ============================================================================
// File Input
// ============================================================================

struct input_file {
    void *data;
    size_t size;
};

// Free resources used by an input file.
void input_file_destroy(struct input_file *file);

// Read an entire file into memory. May use mmap.
int input_file_read(struct input_file *file, const char *filename);
