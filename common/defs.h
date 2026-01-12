// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include <stddef.h>

// Reference to a region of byte data. Does not own its contents.
struct byteslice {
    const void *ptr;
    size_t size;
};

// VADPCM codebook.
struct vadpcm_codebook {
    int order;
    int predictor_count;
    // There are order * predictor_count vectors.
    struct vadpcm_vector *vector;
};
