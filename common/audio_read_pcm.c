// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/audio.h"

#include "codec/vadpcm.h"
#include "common/aiff.h"
#include "common/util.h"

#include <inttypes.h>
#include <string.h>

// Copy 16-bit big-endian samples into the destination.
static void copy_samples(int16_t *dest, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] = ((uint16_t)src[2 * i] << 8) | (uint16_t)src[2 * i + 1];
    }
}

int audio_read_pcm(struct audio_pcm *restrict audio, const char *filename) {
    struct input_file input;
    int r = input_file_read(&input, filename);
    if (r != 0) {
        return -1;
    }
    struct aiff_data aiff;
    r = aiff_parse(&aiff, input.data, input.size);
    if (r != 0) {
        goto error;
    }
    switch (aiff.codec) {
    case kAIFFCodecPCM:
        break;
    default:
        LOG_ERROR("file has unsupported encoding");
        goto error;
    }
    if (aiff.num_channels != 1) {
        LOG_ERROR("only mono files are supported; channels=%" PRIu32,
                  aiff.num_channels);
        goto error;
    }
    if (aiff.sample_size != 16) {
        LOG_ERROR("only 16-bit samples are supported; bits=%" PRIu32,
                  aiff.sample_size);
        goto error;
    }
    if (aiff.num_sample_frames > MAX_INPUT_LENGTH) {
        LOG_ERROR("audio file is too long; length=%" PRIu32
                  ", maximum=%" PRIu32,
                  aiff.num_sample_frames, MAX_INPUT_LENGTH);
        goto error;
    }
    uint32_t original_sample_count = aiff.num_sample_frames;
    uint32_t original_sample_size = original_sample_count * 2;
    if (original_sample_size > aiff.audio.size) {
        LOG_ERROR("audio data is too short; size=%zu, minimum=%" PRIu32,
                  aiff.audio.size, original_sample_size);
        goto error;
    }
    uint32_t padded_sample_count =
        align32(original_sample_count, kVADPCMFrameSampleCount);
    int16_t *sample_data = XMALLOC(padded_sample_count, sizeof(*sample_data));
    copy_samples(sample_data, aiff.audio.ptr, aiff.num_sample_frames);
    memset(sample_data + original_sample_count, 0,
           sizeof(int16_t) * (padded_sample_count - original_sample_count));
    *audio = (struct audio_pcm){
        .meta =
            {
                .original_sample_count = original_sample_count,
                .padded_sample_count = padded_sample_count,
                .sample_rate = aiff.sample_rate,
            },
        .sample_data = sample_data,
    };
    input_file_destroy(&input);
    return 0;

error:
    input_file_destroy(&input);
    return -1;
}
