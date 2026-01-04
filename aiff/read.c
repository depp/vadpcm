// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "aiff/aiff.h"

#include "aiff/internal.h"
#include "util/util.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    uint32_t size = SWAP32_BE(header.chunk_size);
    LOG_DEBUG("size = %" PRIu32, size);
    (void)is_aiffc;
    (void)data;
    fclose(file);
    return 0;
}
