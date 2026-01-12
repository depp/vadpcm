// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include "common/defs.h"
#include "common/extended.h"

#include <stddef.h>
#include <stdint.h>

// FVER timestamp for version 1 of the AIFF-C format. This is the only known
// version.
#define kAIFCVersion1 ((uint32_t)0xA2805140)

// Types of AIFF file.
typedef enum {
    // Original AIFF spec.
    kAIFF,
    // Newer, extended AIFF-C spec.
    kAIFFC,
} aiff_version;

// Supported AIFF codecs.
typedef enum {
    kAIFFCodecPCM,
    kAIFFCodecVADPCM,
} aiff_codec;

// A parsed AIFF or AIFF-C file.
struct aiff_data {
    aiff_version version;
    uint32_t version_timestamp; // FVER for AIFF-C.

    // COMM chunk.
    uint32_t num_channels;
    uint32_t num_sample_frames;
    uint32_t sample_size;
    struct extended sample_rate;
    aiff_codec codec;

    // Sample data in SSND chunk. Note: The size isn't validated against the
    // COMM chunk and may be too small.
    struct byteslice audio;

    // VADPCM codebook. If not present, then the order and predictor count are
    // both zero.
    struct vadpcm_codebook codebook;
};

// Parse an AIFF or AIFF-C file. Returns 0 on success.
int aiff_parse(struct aiff_data *restrict aiff, const uint8_t *ptr,
               size_t size);

// Write out an AIFF or AIFF-C file to disk. Returns 0 on success.
int aiff_write(const struct aiff_data *restrict aiff, const char *filename);
