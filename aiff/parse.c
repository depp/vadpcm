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

// Read an unaligned 32-bit value.
static uint32_t read32(const void *ptr) {
    uint32_t x;
    memcpy(&x, ptr, sizeof(uint32_t));
    return x;
}

static void aiff_parse_comm(struct aiff_data *aiff,
                            const struct aiff_comm *comm) {
    aiff->num_channels = SWAP16_BE(comm->num_channels);
    aiff->num_sample_frames = SWAP32_BE(read32(comm->num_sample_frames));
    aiff->sample_size = SWAP16_BE(comm->sample_size);
    aiff->sample_rate = comm->sample_rate;
}

static int aiff_parse_comm1(struct aiff_data *aiff, const void *ptr,
                            uint32_t size) {
    if (size != 18) {
        LOG_ERROR("COMM chunk too small; size=%" PRIu32 ", minimum=18", size);
        return -1;
    }
    aiff_parse_comm(aiff, ptr);
    aiff->codec = kAIFFCodecPCM;
    return 0;
}

static int aiff_parse_comm2(struct aiff_data *aiff, const void *ptr,
                            uint32_t size) {
    if (size < 23) {
        LOG_ERROR("COMM chunk is too small; size=%" PRIu32 ", minimum=23",
                  size);
        return -1;
    }
    aiff_parse_comm(aiff, ptr);
    uint32_t id = read32((const char *)ptr + 18);
    switch (id) {
    case CODEC_PCM:
        aiff->codec = kAIFFCodecPCM;
        break;
    case CODEC_VADPCM:
        aiff->codec = kAIFFCodecVADPCM;
        break;
    default: {
        char buf[FOURCC_BUFSZ];
        format_fourcc(buf, id);
        LOG_ERROR("unknown codec; id=%s", buf);
        return -1;
    } break;
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
    bool has_ssnd = false;
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
                r = aiff_parse_comm2(aiff, chunk_ptr, chunk_size);
            } else {
                r = aiff_parse_comm1(aiff, chunk_ptr, chunk_size);
            }
            if (r != 0) {
                return r;
            }
            break;
        case CHUNK_SSND:
            if (has_ssnd) {
                LOG_ERROR("multiple SSND chunks found");
                return -1;
            }
            if (chunk_size < 8) {
                LOG_ERROR("SSND chunk is too small; size=%" PRIu32
                          ", minimum=8",
                          chunk_size);
                return -1;
            }
            has_ssnd = true;
            {
                uint32_t offset = SWAP32_BE(read32(chunk_ptr));
                if (offset > chunk_size - 8) {
                    LOG_ERROR("invalid SSND offfset; offset=%" PRIu32, offset);
                    return -1;
                }
                aiff->audio_ptr = (const char *)chunk_ptr + offset + 8;
                aiff->audio_size = chunk_size - 8 - offset;
            }
            break;
        /* case CHUNK_FVER: */
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
    if (!has_ssnd) {
        LOG_ERROR("no SSND chunk");
        return -1;
    }
    if (aiff->sample_size == 0) {
        LOG_ERROR("invalid sample size: 0");
        return -1;
    }
    if (aiff->num_channels == 0) {
        LOG_ERROR("invalid channel count: 0");
        return -1;
    }
    if (aiff->num_sample_frames == 0) {
        LOG_ERROR("invalid frame count: 0");
        return -1;
    }
    switch (aiff->codec) {
    case kAIFFCodecPCM: {
        // 16-bit, so can't overflow (max 8192).
        uint32_t sample_bytes = (aiff->sample_size + 7) >> 3;
        // Also can't overflow: channel count is 16-bit.
        uint32_t frame_bytes = aiff->num_channels * sample_bytes;
        uint32_t audio_bytes;
        if (__builtin_mul_overflow(frame_bytes, aiff->num_sample_frames,
                                   &audio_bytes)) {
            LOG_ERROR("COMM specifies too much audio data");
            return -1;
        }
        if (audio_bytes > aiff->audio_size) {
            LOG_ERROR("SSND data too small; size=%zu, minimum=%" PRIu32,
                      aiff->audio_size, audio_bytes);
            return -1;
        }
    } break;
    case kAIFFCodecVADPCM:
        LOG_ERROR("VADPCM unsupported for now");
        return -1;
    }
    LOG_DEBUG("channels: %" PRIu32, aiff->num_channels);
    LOG_DEBUG("frames: %" PRIu32, aiff->num_sample_frames);
    LOG_DEBUG("bits: %" PRIu32, aiff->sample_size);
    LOG_DEBUG("audio: ptr=%p; size=%zu", aiff->audio_ptr, aiff->audio_size);
    (void)aiff;
    return 0;
}
