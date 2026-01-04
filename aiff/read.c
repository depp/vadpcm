// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "aiff/aiff.h"

#include "aiff/internal.h"
#include "util/util.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// This is a little clumsy.
#if __clang__
#pragma GCC diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

int aiff_read(struct aiff_data *data, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        LOG_ERROR_ERRNO(errno, "open %s", filename);
        return -1;
    }

    // Read the AIFF header.
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
        LOG_DEBUG("file %s: AIFF", filename);
    } else if (header.form_type == AIFF_KIND) {
        is_aiffc = false;
        LOG_DEBUG("file %s: AIFF-C", filename);
    } else {
        LOG_ERROR("read %s: not an AIFF or AIFF-C file", filename);
        return -1;
    }
    uint32_t size = SWAP32_BE(header.chunk_size);
    LOG_DEBUG("size = %" PRIu32, size);
    if (size > LONG_MAX - sizeof(header)) {
        LOG_ERROR("read %s: file is too large", filename);
        return -1;
    }

    uint32_t pos = 4;
    while (pos < size) {
        LOG_DEBUG("pos = %" PRIu32, pos);
        int r = fseek(file, (long)pos + 8, SEEK_SET);
        if (r == -1) {
            LOG_ERROR_ERRNO(errno, "read %s", filename);
            return -1;
        }
        if (sizeof(struct aiff_chunk) > size - pos) {
            LOG_ERROR("read %s: incomplete chunk header", filename);
            return -1;
        }
        struct aiff_chunk chunk;
        amt = fread(&chunk, sizeof(chunk), 1, file);
        if (amt == 0) {
            if (feof(file)) {
                LOG_ERROR("read %s: unexpected EOF", filename);
            } else {
                LOG_ERROR_ERRNO(errno, "read %s", filename);
            }
            return -1;
        }
        pos += sizeof(chunk);
        uint32_t chunk_size = SWAP32_BE(chunk.chunk_size);
        uint32_t chunk_size_padded = chunk_size + (chunk_size & 1);
        if (chunk_size_padded < chunk_size || chunk_size_padded > size - pos) {
            LOG_ERROR("read %s: invalid chunk size", filename);
            return -1;
        }
        switch (chunk.chunk_id) {
        /* case CHUNK_COMM: */
        /*     break; */
        /* case CHUNK_FVER: */
        /*     break; */
        /* case CHUNK_SSND: */
        /*     break; */
        /* case CHUNK_MARK: */
        /*     break; */
        /* case CHUNK_INST: */
        /*     break; */
        /* case CHUNK_APPL: */
        /*     break; */
        default: {
            char buf[FOURCC_BUFSZ];
            format_fourcc(buf, chunk.chunk_id);
            LOG_INFO("unknown chunk: %s", buf);
        } break;
        }
        pos += chunk_size_padded;
    }

    (void)is_aiffc;
    (void)data;
    fclose(file);
    return 0;
}
