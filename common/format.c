// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/format.h"

#include "common/util.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int lower(int c) {
    if ('A' <= c && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

struct extension_text {
    uint32_t ext;
    file_format format;
};

static const struct extension_text kFormatExtensions[] = {
    {FOURCC(0, 'a', 'i', 'f'), kFormatAIFF},
    {FOURCC(0, 'w', 'a', 'v'), kFormatWAVE},
    {FOURCC('a', 'i', 'f', 'f'), kFormatAIFF},
    {FOURCC('a', 'i', 'f', 'c'), kFormatAIFC},
};

file_format format_for_file(const char *filename) {
    size_t size = strlen(filename);
    if (size == 0) {
        return kFormatUnknown;
    }
    const char *p = filename + size;
    do {
        p--;
    } while (p > filename && *p != '.' && *p != '/');
    if (p <= filename || *p != '.' || *(p - 1) == '/') {
        return kFormatUnknown;
    }
    ptrdiff_t esize = (filename + size) - p;
    uint32_t ext;
    switch (esize) {
    case 4:
        ext = ((uint32_t)lower(p[1]) << 16) | ((uint32_t)lower(p[2]) << 8) |
              (uint32_t)lower(p[3]);
        break;
    case 5:
        ext = ((uint32_t)lower(p[1]) << 24) | ((uint32_t)lower(p[2]) << 16) |
              ((uint32_t)lower(p[3]) << 8) | (uint32_t)lower(p[4]);
        break;
    default:
        return kFormatUnknown;
    }
    for (size_t i = 0;
         i < sizeof(kFormatExtensions) / sizeof(*kFormatExtensions); i++) {
        if (kFormatExtensions[i].ext == ext) {
            return kFormatExtensions[i].format;
        }
    }
    return kFormatUnknown;
}

static const char kFormatNames[][7] = {
    [kFormatAIFF] = "AIFF",
    [kFormatAIFC] = "AIFF-C",
    [kFormatWAVE] = "WAVE",
};

// Return the name for a file extension.
const char *name_for_format(file_format fmt) {
    const char *name = "unknown";
    if (0 <= fmt &&
        (size_t)fmt < sizeof(kFormatNames) / sizeof(*kFormatNames)) {
        if (kFormatNames[fmt][0] != '\0') {
            name = kFormatNames[fmt];
        }
    }
    return name;
}
