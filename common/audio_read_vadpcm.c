// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/audio.h"

#include "codec/vadpcm.h"
#include "common/aiff.h"
#include "common/util.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

int audio_read_vadpcm(struct audio_vadpcm *restrict audio,
                      const char *filename) {
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
    if (aiff.codec != kAIFFCodecVADPCM) {
        LOG_ERROR("file does not contain VADPCM data");
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
    uint32_t frame_count =
        (aiff.num_sample_frames + kVADPCMFrameSampleCount - 1) /
        kVADPCMFrameSampleCount;
    uint32_t size = frame_count * kVADPCMFrameByteSize;
    if (aiff.audio.size < size) {
        LOG_ERROR("audio data is too short; size=%zu, expected=%" PRIu32,
                  aiff.audio.size, size);
        goto error;
    }
    // TODO: No copy, just keep input file mapped.
    uint8_t *encoded_data = XMALLOC(size, 1);
    memcpy(encoded_data, aiff.audio.ptr, size);
    *audio = (struct audio_vadpcm){
        .meta =
            {
                .original_sample_count = aiff.num_sample_frames,
                .padded_sample_count = frame_count * kVADPCMFrameSampleCount,
                .sample_rate = aiff.sample_rate,
            },
        .codebook = aiff.codebook,
        .encoded_data = encoded_data,
    };
    input_file_destroy(&input);
    return 0;

error:
    input_file_destroy(&input);
    return -1;
}

void audio_vadpcm_destroy(struct audio_vadpcm *restrict audio) {
    free(audio->encoded_data);
}
