// Copyright 2023 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include <stddef.h>
#include <stdint.h>

struct vadpcm_vector;

// Encode audio as VADPCM, given the assignment of each frame to a predictor.
void vadpcm_encode_data(size_t frame_count, void *restrict dest,
                        const int16_t *restrict src,
                        const uint8_t *restrict predictors,
                        const struct vadpcm_vector *restrict codebook);
