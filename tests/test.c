// Copyright 2022 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "tests/test.h"

#include "codec/vadpcm.h"
#include "common/audio.h"
#include "common/util.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wmultichar"

const char *vadpcm_error_name2(vadpcm_error err) {
    const char *msg = vadpcm_error_name(err);
    return msg == NULL ? "unknown error" : msg;
}

static void print_frame(const int16_t *ptr) {
    for (int i = 0; i < 16; i++) {
        fprintf(stderr, "%8d", ptr[i]);
    }
}

void show_pcm_diff(const int16_t *ref, const int16_t *out) {
    fputs("ref:", stderr);
    print_frame(ref);
    fputc('\n', stderr);

    fputs("out:", stderr);
    print_frame(out);
    fputc('\n', stderr);

    int pos = 0;
    for (int i = 0; i < 16; i++) {
        if (ref[i] != out[i]) {
            int col = 4 + 8 * i;
            while (pos < col) {
                fputc(' ', stderr);
                pos++;
            }
            col += 8;
            while (pos < col) {
                fputc('^', stderr);
                pos++;
            }
        }
    }
    fputc('\n', stderr);
}

const char *const kAIFFNames[] = {
    "sfx1",
    NULL,
};

int test_failure_count;

static void test_file(const char *name) {
    char pcm_path[128], vadpcm_path[128];
    snprintf(pcm_path, sizeof(pcm_path), "tests/data/%s.pcm.aiff", name);
    snprintf(vadpcm_path, sizeof(vadpcm_path), "tests/data/%s.vadpcm.aifc",
             name);

    struct audio_pcm pcm;
    if (audio_read_pcm(&pcm, pcm_path, kFormatAIFF) != 0) {
        test_failure_count++;
        return;
    }
    struct audio_vadpcm vadpcm;
    if (audio_read_vadpcm(&vadpcm, vadpcm_path) != 0) {
        test_failure_count++;
        goto cleanup1;
    }
    if (pcm.meta.original_sample_count != vadpcm.meta.original_sample_count) {
        LOG_ERROR(
            "sample counts do not match; pcm=%" PRIu32 ", vadpcm=%" PRIu32,
            pcm.meta.original_sample_count, vadpcm.meta.original_sample_count);
    }
    size_t frame_count = pcm.meta.padded_sample_count / kVADPCMFrameSampleCount;

    // Run tests.
    test_decode(name, vadpcm.codebook.predictor_count, vadpcm.codebook.order,
                vadpcm.codebook.vector, frame_count, vadpcm.encoded_data,
                pcm.sample_data);
    test_reencode(name, vadpcm.codebook.predictor_count, vadpcm.codebook.order,
                  vadpcm.codebook.vector, frame_count, vadpcm.encoded_data);

    audio_vadpcm_destroy(&vadpcm);
cleanup1:
    audio_pcm_destroy(&pcm);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    test_autocorr();
    test_solve();
    test_stability();
    test_extensions();
    test_extended();
    test_wave();
    test_encode_1();
    for (int i = 0; kAIFFNames[i] != NULL; i++) {
        test_file(kAIFFNames[i]);
    }

    if (test_failure_count > 0) {
        fprintf(stderr, "tests failed: %d\n", test_failure_count);
        return 1;
    }
    fputs("all tests passed\n", stderr);
    return 0;
}
