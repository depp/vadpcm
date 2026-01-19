// Copyright 2023 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/encode.h"
#include "codec/vadpcm.h"
#include "common/util.h"
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
    pcm1 = XMALLOC(sample_count, sizeof(*pcm1));
    err = vadpcm_decode(predictor_count, order, codebook, &state, frame_count,
                        pcm1, vadpcm);
    if (err != 0) {
        fprintf(stderr, "error: test_reencode %s: decode (first): %s", name,
                vadpcm_error_name2(err));
        test_failure_count++;
        goto done;
    }
    predictors = XMALLOC(frame_count, 1);
    for (size_t frame = 0; frame < frame_count; frame++) {
        predictors[frame] =
            ((const uint8_t *)vadpcm)[kVADPCMFrameByteSize * frame] & 15;
    }
    adpcm2 = XMALLOC(frame_count, kVADPCMFrameByteSize);
    struct vadpcm_stats stats;
    struct vadpcm_encoder_state encoder_state = {{0, 0}, 0};
    vadpcm_encode_data(frame_count, adpcm2, pcm1, predictors, codebook, &stats,
                       &encoder_state);
    pcm2 = XMALLOC(kVADPCMFrameSampleCount * frame_count, sizeof(int16_t));
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

struct frame_params {
    int16_t input[16];
    struct vadpcm_vector predictor[2];
    struct vadpcm_encoder_state state;
};

struct frame_result {
    uint8_t output[9];
    int16_t decoded[16];
    vadpcm_error error;
    int16_t estate[2];
    int16_t dstate[2];
};

static void encode_frame(const struct frame_params *params,
                         struct frame_result *result) {
    static const uint8_t zero = 0;
    struct vadpcm_stats stats;
    struct vadpcm_encoder_state encoder_state = params->state;
    vadpcm_encode_data(1, result->output, params->input, &zero,
                       params->predictor, &stats, &encoder_state);
    struct vadpcm_vector state;
    state.v[6] = params->state.data[0];
    state.v[7] = params->state.data[1];
    result->error = vadpcm_decode(1, 2, params->predictor, &state, 1,
                                  result->decoded, result->output);
    for (int i = 0; i < 2; i++) {
        result->estate[i] = encoder_state.data[i];
        result->dstate[i] = state.v[6 + i];
    }
}

void test_encode_1(void) {
    static const struct frame_params params = {
        .input = {27791, 29047, 30145, 31078, 31825, 32364, 32677, 32759, 32619,
                  32281, 31779, 31125, 30314, 29324, 28115, 26647},
        .predictor =
            {
                {{-2036, -4055, -6050, -8016, -9948, -11839, -13685, -15481}},
                {{4078, 6085, 8063, 10005, 11908, 13764, 15570, 17321}},
            },
        .state = {{24842, 26391}, 0x4b1f5080},
    };
    struct frame_result result;
    encode_frame(&params, &result);
    if (result.error != 0) {
        fprintf(stderr, "test_encode_1: %s", vadpcm_error_name(result.error));
        test_failure_count++;
        return;
    }

    if (result.estate[0] != result.dstate[0] ||
        result.estate[1] != result.dstate[1]) {
        fputs("Error: state mismatch\n", stderr);
        test_failure_count++;

        fprintf(stderr, "EState: %d %d\n", result.estate[0], result.estate[1]);
        fprintf(stderr, "DState: %d %d\n", result.dstate[0], result.dstate[1]);

        fputs("Input: ", stderr);
        for (int i = 0; i < 16; i++) {
            fprintf(stderr, " %7d", params.input[i]);
        }
        fputc('\n', stderr);

        fputs("Output:", stderr);
        for (int i = 0; i < 16; i++) {
            fprintf(stderr, " %7d", result.decoded[i]);
        }
        fputc('\n', stderr);

        fputs("Encoded:", stderr);
        for (int i = 0; i < 9; i++) {
            fprintf(stderr, " %02x", result.output[i]);
        }
        fputc('\n', stderr);
    }
}
