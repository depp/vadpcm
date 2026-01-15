// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include "common/defs.h"

#include <stdint.h>

enum {
    kWAVECodecPCM = 1,
    kWAVECodecFloat = 3,
};

// A parsed WAVE file.
struct wave_data {
    // fmt chunk.
    uint16_t codec;
    uint16_t channel_count;
    uint32_t sample_rate;
    uint32_t bytes_per_second;
    uint16_t block_align;
    uint16_t bits_per_sample;

    // data chunk.
    struct byteslice audio;
};

// Parse a WAVE file. Returns 0 on success.
int wave_parse(struct wave_data *restrict wave, const uint8_t *ptr,
               size_t size);

// Write a WAVE file to disk. Returns 0 on success.
int wave_write(struct wave_data *restrict wave, const char *filename);
