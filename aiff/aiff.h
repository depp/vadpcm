// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include "util/extended.h"

#include <stddef.h>
#include <stdint.h>

// Supported AIFF codecs.
typedef enum {
    kAIFFCodecPCM,
    kAIFFCodecVADPCM,
} aiff_codec;

// A parsed AIFF or AIFF-C file.
struct aiff_data {
    uint32_t num_channels;
    uint32_t num_sample_frames;
    uint32_t sample_size;
    struct extended sample_rate;
    aiff_codec codec;

    const void *audio_ptr;
    size_t audio_size;
};

// Parse an AIFF or AIFF-C file. Returns 0 on success.
int aiff_parse(struct aiff_data *aiff, const void *ptr, size_t size);
