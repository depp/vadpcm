// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/aiff.h"

#include "codec/vadpcm.h"
#include "common/aiff_internal.h"
#include "common/binary.h"
#include "common/defs.h"

#include <stdlib.h>
#include <string.h>

// Ordering of chunks in AIFF file.
enum {
    kChunkFVER,
    kChunkCOMM,
    kChunkVCodebook,
    kChunkSSND,

    kChunkCount,
};

static const uint32_t kChunkID[] = {
    [kChunkFVER] = CHUNK_FVER,
    [kChunkCOMM] = CHUNK_COMM,
    [kChunkVCodebook] = CHUNK_APPL,
    [kChunkSSND] = CHUNK_SSND,
};

struct aiff_codec {
    uint32_t fourcc;
    char name[16]; // Length prefix (Pascal-style) string.
};

static const struct aiff_codec kAIFFCodecs[] = {
    [kAIFFCodecPCM] = {CODEC_PCM, "\16not compressed"},
    [kAIFFCodecVADPCM] = {CODEC_VADPCM, "\13VADPCM ~4-1"},
};

int aiff_write(const struct aiff_data *restrict aiff, const char *filename) {
    // Calculate size of each chunk.
    // 0 = not present.
    uint32_t chunk_size[kChunkCount];
    for (int i = 0; i < kChunkCount; i++) {
        chunk_size[i] = 0;
    }
    switch (aiff->version) {
    case kAIFF:
        chunk_size[kChunkCOMM] = 18;
        if (aiff->codec != kAIFFCodecPCM) {
            LOG_ERROR("standard AIFF files must be PCM");
            return -1;
        }
        break;
    case kAIFFC:
        chunk_size[kChunkFVER] = 4;
        chunk_size[kChunkCOMM] =
            23 + (unsigned char)kAIFFCodecs[aiff->codec].name[0];
        if (aiff->codec == kAIFFCodecVADPCM) {
            chunk_size[kChunkVCodebook] =
                16 + 6 +
                16 * aiff->codebook.order * aiff->codebook.predictor_count;
        }
        break;
    }
    if (aiff->audio.size > 0xffffffff - 8) {
        LOG_ERROR("audio data too large");
        return -1;
    }
    chunk_size[kChunkSSND] = 8 + (uint32_t)aiff->audio.size;

    // Calculate location of each chunk.
    uint32_t file_size = 12;
    uint32_t chunk_offset[kChunkCount];
    for (int i = 0; i < kChunkCount; i++) {
        if (chunk_size[i] > 0) {
            chunk_offset[i] = file_size + 8;
            file_size += 8 + align32(chunk_size[i], 2);
        } else {
            chunk_offset[i] = file_size;
        }
    }

    // Fill in chunk headers.
    uint32_t head_size = chunk_offset[kChunkSSND] + 8;
    uint8_t *ptr = XCALLOC(1, head_size);
    uint8_t *cptr;
    write32be(ptr, AIFF_CKID);
    write32be(ptr + 4, file_size - 8);
    for (int i = 0; i < kChunkCount; i++) {
        if (chunk_size[i] > 0) {
            cptr = ptr + chunk_offset[i];
            write32be(cptr - 8, kChunkID[i]);
            write32be(cptr - 4, chunk_size[i]);
        }
    }

    switch (aiff->version) {
    case kAIFF:
        write32be(ptr + 8, AIFF_KIND);
        break;

    case kAIFFC:
        write32be(ptr + 8, AIFC_KIND);

        // FVER chunk.
        cptr = ptr + chunk_offset[kChunkFVER];
        write32be(cptr, kAIFCVersion1);
        break;
    }

    // COMM chunk.
    cptr = ptr + chunk_offset[kChunkCOMM];
    write16be(cptr, aiff->num_channels);
    write32be(cptr + 2, aiff->num_sample_frames);
    write16be(cptr + 6, aiff->sample_size);
    write16be(cptr + 8, aiff->sample_rate.sign_exponent);
    write64be(cptr + 10, aiff->sample_rate.fraction);
    if (aiff->version == kAIFFC) {
        const struct aiff_codec *restrict codec = &kAIFFCodecs[aiff->codec];
        write32be(cptr + 18, codec->fourcc);
        memcpy(cptr + 22, codec->name, (unsigned char)codec->name[0] + 1);

        // VADPCM codebook.
        if (aiff->codec == kAIFFCodecVADPCM) {
            cptr = ptr + chunk_offset[kChunkVCodebook];
            write32be(cptr, APPL_STOC);
            memcpy(cptr + 4, kAPPLCodebook, 12);
            cptr += 16;
            const struct vadpcm_codebook *restrict codebook = &aiff->codebook;
            write16be(cptr, 1); // version
            write16be(cptr + 2, codebook->order);
            write16be(cptr + 4, codebook->predictor_count);
            cptr += 6;
            const struct vadpcm_vector *restrict vector = codebook->vector;
            for (int i = 0, n = codebook->order * codebook->predictor_count;
                 i < n; i++) {
                for (int j = 0; j < 8; j++) {
                    write16be(cptr + 2 * j, vector->v[j]);
                }
                vector++;
                cptr += 16;
            }
        }
    }

    // Create file.
    uint8_t zero = 0;
    struct byteslice chunks[3] = {
        {.ptr = ptr, .size = head_size},
        aiff->audio,
        {.ptr = &zero, .size = chunk_size[kChunkSSND] & 1},
    };
    int r = output_file_write(filename, chunks, 3);
    free(ptr);
    return r;
}
