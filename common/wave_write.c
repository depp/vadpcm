// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/wave.h"

#include "common/binary.h"
#include "common/wave_internal.h"

int wave_write(struct wave_data *restrict wave, const char *filename) {
    uint32_t data_size = align32(wave->audio.size, 2);

    // 12 byte header
    // 8 + 16 byte fmt
    // 8 + N byte data
    uint8_t hdr[44], zero;
    write32be(hdr, WAVE_RIFF);
    write32le(hdr + 4, data_size + 36);
    write32be(hdr + 8, WAVE_WAVE);
    write32be(hdr + 12, WAVE_FMT);
    write32le(hdr + 16, 16);
    write16le(hdr + 20, wave->codec);
    write16le(hdr + 22, wave->channel_count);
    write32le(hdr + 24, wave->sample_rate);
    write32le(hdr + 28, wave->bytes_per_second);
    write16le(hdr + 32, wave->block_align);
    write16le(hdr + 34, wave->bits_per_sample);
    write32be(hdr + 36, WAVE_DATA);
    write32le(hdr + 40, data_size);
    zero = 0;
    struct byteslice chunks[3] = {
        {.ptr = hdr, .size = 44},
        wave->audio,
        {.ptr = &zero, .size = wave->audio.size & 1},
    };
    return output_file_write(filename, chunks, 3);
}
