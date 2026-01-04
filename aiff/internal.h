// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "util/util.h"

#include <stdint.h>

#define AIFF_CKID FOURCC_BE('F', 'O', 'R', 'M')
#define AIFF_KIND FOURCC_BE('A', 'I', 'F', 'F')
#define AIFC_KIND FOURCC_BE('A', 'I', 'F', 'C')

struct aiff_header {
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t form_type;
};
