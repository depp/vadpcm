// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include "common/defs.h"
#include <stddef.h>
#include <stdint.h>

struct byteslice;

// ============================================================================
// Logging
// ============================================================================

typedef enum {
    LEVEL_ERROR,
    LEVEL_INFO,
    LEVEL_DEBUG,
} log_level;

#define LEVEL_QUIET LEVEL_ERROR

extern log_level g_log_level;

// Show an error message.
void log_error(const char *file, int line, const char *fmt, ...)
    ATTRIBUTE((format(printf, 3, 4)));

// Show an error message, with an error code from errno appended.
void log_error_errno(const char *file, int line, int errcode, const char *fmt,
                     ...) ATTRIBUTE((format(printf, 4, 5)));

// Show an informational message.
void log_info(const char *file, int line, const char *fmt, ...)
    ATTRIBUTE((format(printf, 3, 4)));

// Show a debugging message.
void log_debug(const char *file, int line, const char *fmt, ...)
    ATTRIBUTE((format(printf, 3, 4)));

#define LOG_ERROR(...) log_error(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR_ERRNO(errcode, ...) \
    log_error_errno(__FILE__, __LINE__, errcode, __VA_ARGS__)
#define LOG_INFO(...) log_info(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_debug(__FILE__, __LINE__, __VA_ARGS__)

// Set additional context for logging. This will be printed before each log
// message.
void log_context(const char *operation, const char *path);

// Clear additional context for logging.
void log_context_clear(void);

// ============================================================================
// Memory Allocation
// ============================================================================

void *xmalloc(const char *file, int line, size_t nmemb, size_t size)
    ATTRIBUTE((malloc, alloc_size(3, 4)));
void *xcalloc(const char *file, int line, size_t nmemb, size_t size)
    ATTRIBUTE((malloc, alloc_size(3, 4)));

#define XMALLOC(nmemb, size) xmalloc(__FILE__, __LINE__, nmemb, size)
#define XCALLOC(nmemb, size) xcalloc(__FILE__, __LINE__, nmemb, size)

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
// File Input and Output
// ============================================================================

struct input_file {
    void *data;
    size_t size;
};

// Free resources used by an input file.
void input_file_destroy(struct input_file *file);

// Read an entire file into memory. May use mmap.
int input_file_read(struct input_file *file, const char *filename);

// Write an output file, consisting of one or more parts.
int output_file_write(const char *filename, const struct byteslice *data,
                      size_t count);

// ============================================================================
// Misc
// ============================================================================

// Pad a value to be a multiple of the alignment, which must be a power of
// two.
inline uint32_t align32(uint32_t value, uint32_t alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}
