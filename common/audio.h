// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include "common/defs.h"
#include "common/extended.h"

#include <stdint.h>

// Maximum number of samples in an input file. This limit is somewhat
// arbitrary for now. It means that we don't overflow a 32-bit number when
// calculating sizes with 16-bit samples, and it's a power of two so we wont
// go over it because of padding.
#define MAX_INPUT_LENGTH ((uint32_t)0x40000000)

// Metadata for audio files. For convenience, the audio data is always padded
// with zeroes to a multiple of the VADPCM frame size.
struct audio_meta {
    uint32_t original_sample_count;
    uint32_t padded_sample_count;
    struct extended sample_rate;
};

// Audio data as 16-bit native PCM. Whenever we read PCM data, the data is
// padded with zeroes to a boundary of a VADPCM frame. Both the original and
// the padded sample length are recorded.
struct audio_pcm {
    struct audio_meta meta;
    int16_t *sample_data;
};

// Read PCM audio from a file.
int audio_read_pcm(struct audio_pcm *restrict audio, const char *filename);

// VADPCM-encoded audio data.
struct audio_vadpcm {
    struct audio_meta meta;
    struct vadpcm_codebook codebook;
    uint8_t *encoded_data;
};

// Read VADPCM audio from a afile.
int audio_read_vadpcm(struct audio_vadpcm *restrict audio,
                      const char *filename);
