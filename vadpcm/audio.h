// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include "util/extended.h"

#include <stdint.h>

// Audio data as 16-bit native PCM. Whenever we read PCM data, the data is
// padded with zeroes to a boundary of a VADPCM frame. Both the original and
// the padded sample length are recorded.
struct audio_data {
    uint32_t original_sample_count;
    uint32_t padded_sample_count;
    int16_t *sample_data;
    struct extended sample_rate;
};

// Read PCM audio from a file.
int audio_read_pcm(struct audio_data *restrict data, const char *filename);
