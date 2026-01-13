// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/aiff.h"

#include "codec/vadpcm.h"
#include "common/aiff_internal.h"
#include "common/binary.h"
#include "common/util.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// This is a little clumsy.
#if __clang__
#pragma GCC diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

static void aiff_parse_comm(struct aiff_data *aiff, const uint8_t *ptr) {
    aiff->num_channels = read16be(ptr);
    aiff->num_sample_frames = read32be(ptr + 2);
    aiff->sample_size = read16be(ptr + 6);
    aiff->sample_rate.sign_exponent = read16be(ptr + 8);
    aiff->sample_rate.fraction = read64be(ptr + 10);
}

static int aiff_parse_comm1(struct aiff_data *aiff, const uint8_t *ptr,
                            uint32_t size) {
    if (size != 18) {
        LOG_ERROR("COMM chunk too small; size=%" PRIu32 ", minimum=18", size);
        return -1;
    }
    aiff_parse_comm(aiff, ptr);
    aiff->codec = kAIFFCodecPCM;
    return 0;
}

static int aiff_parse_comm2(struct aiff_data *aiff, const uint8_t *ptr,
                            uint32_t size) {
    if (size < 23) {
        LOG_ERROR("COMM chunk is too small; size=%" PRIu32 ", minimum=23",
                  size);
        return -1;
    }
    aiff_parse_comm(aiff, ptr);
    uint32_t id = read32be(ptr + 18);
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

static int aiff_parse_codebook(struct vadpcm_codebook *restrict codebook,
                               const uint8_t *ptr, uint32_t size) {
    if (size < 2) {
        LOG_ERROR("codebook too short; size=%" PRIu32, size);
        return -1;
    }
    uint16_t version = read16be(ptr);
    if (version != 1) {
        LOG_ERROR("codebook has unknown version; version=%" PRIu16, version);
        return -1;
    }
    if (size < 6) {
        LOG_ERROR("codebook too short; size=%" PRIu32, size);
        return -1;
    }
    uint16_t order = read16be(ptr + 2);
    uint16_t predictor_count = read16be(ptr + 4);
    if (order > kVADPCMMaxOrder) {
        LOG_ERROR("codebook order is too large; order=%" PRIu16 ", maximum=%u",
                  order, kVADPCMMaxOrder);
        return -1;
    }
    if (predictor_count > kVADPCMMaxPredictorCount) {
        LOG_ERROR("predictor count is too large; count=%" PRIu16 ", maximum=%u",
                  predictor_count, kVADPCMMaxPredictorCount);
        return -1;
    }
    // Can't overflow, maximum value is 128.
    uint32_t vector_count = order * predictor_count;
    struct vadpcm_vector *vector = NULL;
    if (vector_count > 0) {
        // Can't overflow, maximum value is 2048.
        if (6 + vector_count * 16 > size) {
            LOG_ERROR("codebook is too short; size=%" PRIu32, size);
            return -1;
        }
        vector = XMALLOC(vector_count, sizeof(*vector));
        const uint8_t *vptr = ptr + 6;
        for (uint32_t i = 0; i < vector_count; i++) {
            for (int j = 0; j < kVADPCMVectorSampleCount; j++) {
                vector[i].v[j] = read16be(vptr);
                vptr += 2;
            }
        }
    }
    *codebook = (struct vadpcm_codebook){
        .order = order,
        .predictor_count = predictor_count,
        .vector = vector,
    };
    return 0;
}

int aiff_parse(struct aiff_data *restrict aiff, const uint8_t *ptr,
               size_t size) {
    // Read the header.
    if (size < 12) {
        LOG_ERROR("file size is too small; size=%zu, minimum=12", size);
        return -1;
    }
    uint32_t chunk_id = read32be(ptr);
    if (chunk_id != AIFF_CKID) {
        char buf[FOURCC_BUFSZ];
        format_fourcc(buf, chunk_id);
        LOG_ERROR("bad container chunk; id=%s, expected='FORM'", buf);
        return -1;
    }
    uint32_t form_type = read32be(ptr + 8);
    bool is_aiffc;
    if (form_type == AIFF_KIND) {
        is_aiffc = false;
        aiff->version = kAIFF;
        LOG_DEBUG("type: AIFF");
    } else if (form_type == AIFC_KIND) {
        is_aiffc = true;
        aiff->version = kAIFFC;
        LOG_DEBUG("type: AIFF-C");
    } else {
        char buf[FOURCC_BUFSZ];
        format_fourcc(buf, form_type);
        LOG_ERROR("form type is not 'AIFF' or 'AIFC'; type=%s", buf);
        return -1;
    }
    aiff->version_timestamp = 0;
    uint32_t content_size = read32be(ptr + 4);
    LOG_DEBUG("size=%" PRIu32, content_size);
    if (content_size > size - 8) {
        LOG_ERROR("short AIFF file; body size=%" PRIu32 ", file size=%zu",
                  content_size, size);
        return -1;
    }

    // Read all the chunks in the file.
    ptrdiff_t offset = 12;
    ptrdiff_t end = (ptrdiff_t)content_size + 8;
    bool has_comm = false;
    bool has_ssnd = false;
    bool has_codebook = false;
    while (offset < end) {
        if (8 > end - offset) {
            LOG_ERROR("incomplete chunk header; offset=%td", offset);
            return -1;
        }
        const uint8_t *cptr = ptr + offset;
        chunk_id = read32be(cptr);
        uint32_t chunk_size = read32be(cptr + 4);
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
        case CHUNK_COMM:
            if (has_comm) {
                LOG_ERROR("multiple COMM chunks found");
                return -1;
            }
            has_comm = true;
            int r;
            if (is_aiffc) {
                r = aiff_parse_comm2(aiff, cptr, chunk_size);
            } else {
                r = aiff_parse_comm1(aiff, cptr, chunk_size);
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
                uint32_t ssnd_offset = read32be(cptr);
                if (ssnd_offset > chunk_size - 8) {
                    LOG_ERROR("invalid SSND offfset; offset=%" PRIu32,
                              ssnd_offset);
                    return -1;
                }
                aiff->audio = (struct byteslice){
                    .ptr = cptr + ssnd_offset + 8,
                    .size = chunk_size - 8 - ssnd_offset,
                };
            }
            break;
        case CHUNK_FVER:
            if (chunk_size < 4) {
                LOG_ERROR("FVER chunk is too small; size=%" PRIu32
                          ", minimum=4",
                          chunk_size);
                return -1;
            }
            aiff->version_timestamp = read32be(cptr);
            break;
        /* case CHUNK_MARK: */
        /*     break; */
        /* case CHUNK_INST: */
        /*     break; */
        case CHUNK_APPL: {
            LOG_DEBUG("APPL CHUNK");
            if (chunk_size < 4) {
                LOG_ERROR("APPL chunk is too small; size=%" PRIu32
                          ", minimum=4",
                          chunk_size);
                return -1;
            }
            uint32_t signature = read32be(cptr);
            if (signature == APPL_STOC) {
                LOG_DEBUG("is stoc");
                if (chunk_size < 5) {
                    LOG_ERROR("APPL stoc chunk is too small; size=%" PRIu32
                              ", minimum=5",
                              chunk_size);
                    return -1;
                }
                const uint8_t *nptr = cptr + 4;
                uint32_t nsize = chunk_size - 4;
                uint32_t name_length = nptr[0];
                uint32_t padded_name_length = align32(name_length, 2);
                if (nsize < padded_name_length) {
                    LOG_ERROR("APPL stoc chunk is truncated");
                    return -1;
                }
                const uint8_t *aptr = nptr + padded_name_length;
                uint32_t asize = nsize - padded_name_length;
                if (name_length == 11) {
                    if (memcmp(nptr, kAPPLCodebook, 12) == 0) {
                        if (has_codebook) {
                            LOG_ERROR("multiple codebooks found");
                            return -1;
                        }
                        has_codebook = true;
                        if (aiff_parse_codebook(&aiff->codebook, aptr, asize) !=
                            0) {
                            return -1;
                        }
                    }
                }
            }
        } break;
        default:
            if (g_log_level >= LEVEL_DEBUG) {
                char buf[FOURCC_BUFSZ];
                format_fourcc(buf, chunk_id);
                LOG_DEBUG("unknown chunk: %s", buf);
            }
            break;
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
    if (aiff->codec == kAIFFCodecVADPCM && !has_codebook) {
        LOG_ERROR("no codebook");
        return -1;
    }
    LOG_DEBUG("channels: %" PRIu32, aiff->num_channels);
    LOG_DEBUG("frames: %" PRIu32, aiff->num_sample_frames);
    LOG_DEBUG("bits: %" PRIu32, aiff->sample_size);
    LOG_DEBUG("audio: ptr=%p; size=%zu", aiff->audio.ptr, aiff->audio.size);
    return 0;
}
