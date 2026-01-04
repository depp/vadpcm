// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include <stddef.h>
#include <stdint.h>

// A parsed AIFF or AIFF-C file.
struct aiff_data {
    int16_t *samples;
};

// Parse an AIFF or AIFF-C file. Returns 0 on success.
int aiff_parse(struct aiff_data *aiff, const void *ptr, size_t size);
