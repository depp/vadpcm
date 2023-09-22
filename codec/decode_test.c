// Copyright 2022 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/test.h"
#include "codec/vadpcm.h"

#include <stdio.h>
#include <stdlib.h>

void test_decode(const char *name, int predictor_count, int order,
                 struct vadpcm_vector *codebook, size_t frame_count,
                 const void *vadpcm, const int16_t *pcm) {
    size_t sample_count = frame_count * kVADPCMFrameSampleCount;
    int16_t *out_pcm = xmalloc(sizeof(*out_pcm) * sample_count);
    struct vadpcm_vector state = {{0}};
    vadpcm_error err = vadpcm_decode(predictor_count, order, codebook, &state,
                                     frame_count, out_pcm, vadpcm);
    if (err != 0) {
        fprintf(stderr, "error: test_decode %s: %s", name,
                vadpcm_error_name2(err));
        test_failure_count++;
        goto done;
    }
    for (size_t i = 0; i < sample_count; i++) {
        if (pcm[i] != out_pcm[i]) {
            fprintf(stderr,
                    "error: test_decode %s: output does not match, "
                    "index = %zu\n",
                    name, i);
            size_t frame = i / kVADPCMFrameSampleCount;
            show_pcm_diff(pcm + frame * kVADPCMFrameSampleCount,
                          out_pcm + frame * kVADPCMFrameSampleCount);
            test_failure_count++;
            goto done;
        }
    }
done:
    free(out_pcm);
}
