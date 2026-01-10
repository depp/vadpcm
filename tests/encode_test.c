// Copyright 2023 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/encode.h"
#include "tests/test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int vadpcm_ext4(int x) {
    return x > 7 ? x - 16 : x;
}

static void show_raw(const char *name, const uint8_t *data) {
    fprintf(stderr, "%s: scale = %d, predictor = %d\n%s:", name, data[0] >> 4,
            data[0] & 15, name);
    for (int i = 1; i < kVADPCMFrameByteSize; i++) {
        fprintf(stderr, "%8d%8d", vadpcm_ext4(data[i] >> 4),
                vadpcm_ext4(data[i] & 15));
    }
    fputc('\n', stderr);
}

void test_reencode(const char *name, int predictor_count, int order,
                   struct vadpcm_vector *codebook, size_t frame_count,
                   const void *vadpcm) {
    // Take a VADPCM file. Decode it, then reencode it with the same codebook,
    // and then decode it again. The decoded PCM data should match.
    int16_t *pcm1 = NULL, *pcm2 = NULL;
    uint8_t *adpcm2 = NULL;
    uint8_t *predictors = NULL;
    vadpcm_error err;
    size_t sample_count = frame_count * kVADPCMFrameSampleCount;

    struct vadpcm_vector state;
    memset(&state, 0, sizeof(state));
    pcm1 = xmalloc(sizeof(*pcm1) * sample_count);
    err = vadpcm_decode(predictor_count, order, codebook, &state, frame_count,
                        pcm1, vadpcm);
    if (err != 0) {
        fprintf(stderr, "error: test_reencode %s: decode (first): %s", name,
                vadpcm_error_name2(err));
        test_failure_count++;
        goto done;
    }
    predictors = xmalloc(frame_count);
    for (size_t frame = 0; frame < frame_count; frame++) {
        predictors[frame] =
            ((const uint8_t *)vadpcm)[kVADPCMFrameByteSize * frame] & 15;
    }
    adpcm2 = xmalloc(kVADPCMFrameByteSize * frame_count);
    struct vadpcm_stats stats;
    vadpcm_encode_data(frame_count, adpcm2, pcm1, predictors, codebook, &stats);
    pcm2 = xmalloc(kVADPCMFrameSampleCount * sizeof(int16_t) * frame_count);
    memset(&state, 0, sizeof(state));
    err = vadpcm_decode(predictor_count, order, codebook, &state, frame_count,
                        pcm2, adpcm2);
    if (err != 0) {
        fprintf(stderr, "error: test_reencode %s: decode (second): %s", name,
                vadpcm_error_name2(err));
        test_failure_count++;
        goto done;
    }
    for (size_t i = 0; i < frame_count * kVADPCMFrameSampleCount; i++) {
        if (pcm1[i] != pcm2[i]) {
            fprintf(stderr,
                    "error: reencode %s: output does not match, "
                    "index = %zu\n",
                    name, i);
            size_t frame = i / kVADPCMFrameSampleCount;
            fputs("vadpcm:\n", stderr);
            show_raw("raw",
                     (const uint8_t *)vadpcm + kVADPCMFrameByteSize * frame);
            show_raw("out", adpcm2 + kVADPCMFrameByteSize * frame);
            fputs("pcm:\n", stderr);
            show_pcm_diff(pcm1 + frame * kVADPCMFrameSampleCount,
                          pcm2 + frame * kVADPCMFrameSampleCount);
            test_failure_count++;
            goto done;
        }
    }

done:
    free(pcm1);
    free(pcm2);
    free(adpcm2);
    free(predictors);
}
