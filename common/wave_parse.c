// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/wave.h"

#include "common/binary.h"
#include "common/util.h"
#include "common/wave_internal.h"

#include <inttypes.h>
#include <stdbool.h>

// TODO: Parse "advanced" version of PCM type
// 00000001 00 00 10 00 80 00 00 aa 00 38 9b 71 - PCM
// 00000003 00 00 10 00 80 00 00 aa 00 38 9b 71 - Float

int wave_parse(struct wave_data *restrict wave, const uint8_t *ptr,
               size_t size) {
    if (size < 12) {
        LOG_ERROR("not a WAVE file");
        return -1;
    }
    uint32_t riff_sig = read32be(ptr);
    uint32_t riff_type = read32be(ptr + 8);
    if (riff_sig != WAVE_RIFF || riff_type != WAVE_WAVE) {
        char sig[FOURCC_BUFSZ], type[FOURCC_BUFSZ];
        format_fourcc(sig, riff_sig);
        format_fourcc(type, riff_type);
        LOG_ERROR("not a WAVE file; signature=%s, type=%s", sig, type);
        return -1;
    }
    uint32_t content_size = read32le(ptr + 4);
    if (content_size < 4 || content_size > size - 8) {
        LOG_ERROR("short WAVE file; body size=%" PRIu32 ", file size=%zu",
                  content_size, size);
        return -1;
    }

    // Read all the chunks in the file.
    ptrdiff_t offset = 12;
    ptrdiff_t end = (ptrdiff_t)content_size + 8;
    bool has_fmt = false;
    bool has_data = false;
    while (offset < end) {
        if (8 > end - offset) {
            LOG_ERROR("incomplete chunk header; offset=%td", offset);
            return -1;
        }
        const uint8_t *cptr = ptr + offset;
        uint32_t chunk_id = read32be(cptr);
        uint32_t chunk_size = read32le(cptr + 4);
        uint32_t chunk_size_padded = align32(chunk_size, 2);
        offset += 8;
        cptr += 8;
        if (chunk_size_padded < chunk_size ||
            chunk_size_padded > end - offset) {
            LOG_ERROR("invalid chunk size; offset=%td, size=%" PRIu32, offset,
                      chunk_size);
            return -1;
        }
        switch (chunk_id) {
        case WAVE_FMT:
            if (has_fmt) {
                LOG_ERROR("multiple fmt chunks");
                goto error;
            }
            if (chunk_size < 16) {
                LOG_ERROR("fmt chunk is too small; size=%" PRIu32 ", minimum=8",
                          chunk_size);
                goto error;
            }
            has_fmt = true;
            wave->codec = read16le(cptr);
            wave->channel_count = read16le(cptr + 2);
            wave->sample_rate = read32le(cptr + 4);
            wave->bytes_per_second = read32le(cptr + 8);
            wave->block_align = read16le(cptr + 12);
            wave->bits_per_sample = read16le(cptr + 14);
            break;
        case WAVE_FACT:
            break;
        case WAVE_DATA:
            if (has_data) {
                LOG_ERROR("multiple data chunks");
                goto error;
            }
            has_data = true;
            wave->audio = (struct byteslice){
                .ptr = cptr,
                .size = chunk_size,
            };
            break;
        default:
            if (g_log_level >= LEVEL_DEBUG) {
                char buf[FOURCC_BUFSZ];
                format_fourcc(buf, chunk_id);
                LOG_DEBUG("unknown chunk: %s", buf);
            }
        }
        offset += chunk_size_padded;
    }
    if (!has_fmt) {
        LOG_ERROR("missing required fmt chunk");
        goto error;
    }
    if (!has_data) {
        LOG_ERROR("missing required data chunk");
        goto error;
    }
    return 0;

error:
    return -1;
}
