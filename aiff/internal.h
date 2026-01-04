// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include "aiff/aiff.h"
#include "util/util.h"

#include <stdint.h>

#define AIFF_CKID FOURCC_BE('F', 'O', 'R', 'M')
#define AIFF_KIND FOURCC_BE('A', 'I', 'F', 'F')
#define AIFC_KIND FOURCC_BE('A', 'I', 'F', 'C')

#define CHUNK_COMM FOURCC_BE('C', 'O', 'M', 'M')
#define CHUNK_FVER FOURCC_BE('F', 'V', 'E', 'R')
#define CHUNK_SSND FOURCC_BE('S', 'S', 'N', 'D')
#define CHUNK_MARK FOURCC_BE('M', 'A', 'R', 'K')
#define CHUNK_INST FOURCC_BE('I', 'N', 'S', 'T')
#define CHUNK_APPL FOURCC_BE('A', 'P', 'P', 'L')

#define CODEC_PCM FOURCC_BE('N', 'O', 'N', 'E')
#define CODEC_VADPCM FOURCC_BE('V', 'A', 'P', 'C')

struct aiff_header {
    uint32_t chunk_id;
    uint32_t chunk_size;
    uint32_t form_type;
};

struct aiff_chunk {
    uint32_t chunk_id;
    uint32_t chunk_size;
};

// COMM chunk for AIFF.
struct aiff_comm {
    // Number of channels.
    uint16_t num_channels;
    // Number of frames. Each channel has one sample per frame.
    uint16_t num_sample_frames[2]; // Unaligned uint32.
    // Size of each sample, in bits.
    uint16_t sample_size;
    // Sample rate, in Hz.
    struct extended sample_rate;
};

// COMM chunk for AIFF-C.
struct aiff_comm2 {
    struct aiff_comm comm;
    uint16_t compression_type[2];  // Unaligned uint32.
    uint8_t compression_name[256]; // Pascal style string.
};
