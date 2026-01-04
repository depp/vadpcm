// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "aiff/aiff.h"

#include "aiff/internal.h"
#include "util/util.h"

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// This is a little clumsy.
#if __clang__
#pragma GCC diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

enum {
    FORMAT_PCM,
    FORMAT_VADPCM,
};

struct aiff_comminfo {
    uint32_t num_channels;
    uint32_t num_sample_frames;
    uint32_t sample_size;
    struct extended sample_rate;
    int format;
};

// Read an unaligned 32-bit value.
static uint32_t read32(const void *ptr) {
    uint32_t x;
    memcpy(&x, ptr, sizeof(uint32_t));
    return x;
}

static void aiff_parse_comm(struct aiff_comminfo *info,
                            const struct aiff_comm *comm) {
    info->num_channels = SWAP16_BE(comm->num_channels);
    info->num_sample_frames = SWAP32_BE(read32(comm->num_sample_frames));
    info->sample_size = SWAP16_BE(comm->sample_size);
    info->sample_rate = comm->sample_rate;
}

static int aiff_read_comm(struct aiff_comminfo *info, const void *ptr,
                          uint32_t size) {
    if (size != 18) {
        LOG_ERROR("invalid common chunk: len = %" PRIu32
                  ", should be 18 or more",
                  size);
        return -1;
    }
    aiff_parse_comm(info, ptr);
    info->format = FORMAT_PCM;
    return 0;
}

static int aiff_read_comm2(struct aiff_comminfo *info, const void *ptr,
                           uint32_t size) {
    if (size < 23) {
        LOG_ERROR("invalid common chunk: len = %" PRIu32
                  ", should be 23 or more",
                  size);
        return -1;
    }
    aiff_parse_comm(info, ptr);
    uint32_t id = read32((const char *)ptr + 18);
    if (id == CODEC_PCM) {
        info->format = FORMAT_PCM;
    } else {
        char buf[FOURCC_BUFSZ];
        format_fourcc(buf, id);
        LOG_ERROR("unknown codec; id=%s", buf);
        return -1;
    }
    return 0;
}

int aiff_parse(struct aiff_data *aiff, const void *ptr, size_t size) {
    // Read the header.
    _Static_assert(sizeof(struct aiff_header) == 12, "must be 12");
    if (size < 12) {
        LOG_ERROR("file size is too small; size=%zu, minimum=12", size);
        return -1;
    }
    const struct aiff_header *header = ptr;
    if (header->chunk_id != AIFF_CKID) {
        char buf[FOURCC_BUFSZ];
        format_fourcc(buf, header->chunk_id);
        LOG_ERROR("bad container chunk; id=%s, expected='FORM'", buf);
        return -1;
    }
    bool is_aiffc;
    if (header->form_type == AIFC_KIND) {
        is_aiffc = true;
        LOG_DEBUG("type: AIFF");
    } else if (header->form_type == AIFF_KIND) {
        is_aiffc = false;
        LOG_DEBUG("type: AIFF-C");
    } else {
        char buf[FOURCC_BUFSZ];
        format_fourcc(buf, header->form_type);
        LOG_ERROR("form type is not 'AIFF' or 'AIFC'; type=%s", buf);
        return -1;
    }
    uint32_t content_size = SWAP32_BE(header->chunk_size);
    LOG_DEBUG("size=%" PRIu32, content_size);
    if (content_size > size - 8) {
        LOG_ERROR("short AIFF file; body size=%" PRIu32 ", file size=%zu",
                  content_size, size);
        return -1;
    }

    // Read all the chunks in the file.
    ptrdiff_t offset = 12;
    ptrdiff_t end = content_size + 8;
    bool has_comm = false;
    struct aiff_comminfo comm;
    while (offset < end) {
        if ((ptrdiff_t)sizeof(struct aiff_chunk) > end - offset) {
            LOG_ERROR("incomplete chunk header; offset=%td", offset);
            return -1;
        }
        const struct aiff_chunk *chunk =
            (const void *)((const char *)ptr + offset);
        offset += sizeof(struct aiff_chunk);
        uint32_t chunk_size = SWAP32_BE(chunk->chunk_size);
        uint32_t chunk_size_padded = chunk_size + (chunk_size & 1);
        if (chunk_size_padded < chunk_size ||
            chunk_size_padded > end - offset) {
            LOG_ERROR("invalid chunk size; offset=%td, size=%" PRIu32, offset,
                      chunk_size);
            return -1;
        }
        const void *chunk_ptr = (const char *)ptr + offset;
        switch (chunk->chunk_id) {
        case CHUNK_COMM:
            if (has_comm) {
                LOG_ERROR("multiple COMM chunks found");
                return -1;
            }
            has_comm = true;
            int r;
            if (is_aiffc) {
                r = aiff_read_comm2(&comm, chunk_ptr, chunk_size);
            } else {
                r = aiff_read_comm(&comm, chunk_ptr, chunk_size);
            }
            if (r != 0) {
                return r;
            }
            break;
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
            format_fourcc(buf, chunk->chunk_id);
            LOG_INFO("unknown chunk: %s", buf);
        } break;
        }
        offset += chunk_size_padded;
    }
    if (!has_comm) {
        LOG_ERROR("no COMM chunk");
        return -1;
    }
    LOG_DEBUG("channels: %" PRIu32, comm.num_channels);
    LOG_DEBUG("frames: %" PRIu32, comm.num_sample_frames);
    LOG_DEBUG("bits: %" PRIu32, comm.sample_size);
    (void)aiff;
    return 0;
}
