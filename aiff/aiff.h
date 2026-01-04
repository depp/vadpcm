// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include <stddef.h>
#include <stdint.h>

struct aiff_data {
    int16_t *samples;
};

// Read an AIFF or AIFF-C file. Returns 0 on success.
int aiff_read(struct aiff_data *data, const char *filename);
