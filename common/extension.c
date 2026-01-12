// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/extension.h"

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
    extension value;
};

static const struct extension_text kExtensions[] = {
    {FOURCC(0, 'a', 'i', 'f'), kExtAIFF},
    {FOURCC(0, 'w', 'a', 'v'), kExtWAVE},
    {FOURCC('a', 'i', 'f', 'f'), kExtAIFF},
    {FOURCC('a', 'i', 'f', 'c'), kExtAIFC},
};

extension extension_for_file(const char *filename) {
    size_t size = strlen(filename);
    if (size == 0) {
        return kExtUnknown;
    }
    const char *p = filename + size;
    do {
        p--;
    } while (p > filename && *p != '.' && *p != '/');
    if (p <= filename || *p != '.' || *(p - 1) == '/') {
        return kExtUnknown;
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
        return kExtUnknown;
    }
    for (size_t i = 0; i < sizeof(kExtensions) / sizeof(*kExtensions); i++) {
        if (kExtensions[i].ext == ext) {
            return kExtensions[i].value;
        }
    }
    return kExtUnknown;
}

static const char kExtensionNames[][7] = {
    [kExtAIFF] = "AIFF",
    [kExtAIFC] = "AIFF-C",
    [kExtWAVE] = "WAVE",
};

// Return the name for a file extension.
const char *name_for_extension(extension ext) {
    const char *name = "unknown";
    if (0 <= ext &&
        (size_t)ext < sizeof(kExtensionNames) / sizeof(*kExtensionNames)) {
        if (kExtensionNames[ext][0] != '\0') {
            name = kExtensionNames[ext];
        }
    }
    return name;
}
