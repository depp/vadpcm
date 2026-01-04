// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "aiff/aiff.h"

#include "util/util.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FOURCC_NATIVE(c1, c2, c3, c4)                                          \
    (((uint32_t)(c1) << 24) | ((uint32_t)(c2) << 16) | ((uint32_t)(c3) << 8) | \
     (uint32_t)(c4))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FOURCC(c1, c2, c3, c4) FOURCC_NATIVE(c4, c3, c2, c1)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FOURCC(c1, c2, c3, c4) FOURCC_NATIVE(c1, c2, c3, c4)
#else
#error "Unknown endian"
#endif

#define AIFF_CKID FOURCC('F', 'O', 'R', 'M')
#define AIFF_KIND FOURCC('A', 'I', 'F', 'F')
#define AIFC_KIND FOURCC('A', 'I', 'F', 'C')

struct aiff_header {
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t form_type;
};

int aiff_read(struct aiff_data *data, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        LOG_ERROR_ERRNO(errno, "open %s", filename);
        return -1;
    }
    struct aiff_header header;
    size_t amt = fread(&header, sizeof(header), 1, file);
    if (amt == 0) {
        if (feof(file)) {
            LOG_ERROR("read %s: not an AIFF or AIFF-C file", filename);
        } else {
            LOG_ERROR_ERRNO(errno, "read %s", filename);
        }
        return -1;
    }
    if (header.chunk_id != AIFF_CKID) {
        LOG_ERROR("read %s: not an AIFF or AIFF-C file", filename);
        return -1;
    }
    bool is_aiffc;
    if (header.form_type == AIFC_KIND) {
        is_aiffc = true;
    } else if (header.form_type == AIFF_KIND) {
        is_aiffc = false;
    } else {
        LOG_ERROR("read %s: not an AIFF or AIFF-C file", filename);
        return -1;
    }
    (void)is_aiffc;
    (void)data;
    fclose(file);
    return 0;
}
