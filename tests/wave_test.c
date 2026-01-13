// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "tests/test.h"

#include "common/util.h"
#include "common/wave.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct wave_file {
    char name[12];
    uint16_t codec;
    uint16_t bits_per_sample;
};

static const struct wave_file kWAVEFiles[] = {
    {"wave8.wav", kWAVECodecPCM, 8},
    {"wave16.wav", kWAVECodecPCM, 16},
    // {"wave24.wav", kWAVECodecPCM, 24},
    {"wave32.wav", kWAVECodecFloat, 32},
};

static bool test_wave1(const struct wave_file *restrict info) {
    char path[40];
    snprintf(path, sizeof(path), "tests/formats/%s", info->name);
    log_context("wave %s", path);
    struct input_file file;
    int r = input_file_read(&file, path);
    if (r != 0) {
        return false;
    }
    bool ok = false;
    struct wave_data wave;
    r = wave_parse(&wave, file.data, file.size);
    if (r != 0) {
        goto done;
    }
    ok = true;
    if (wave.codec != info->codec) {
        LOG_ERROR("codec=%" PRIu16 ", expect=%" PRIu16, wave.codec,
                  info->codec);
        ok = false;
    }
    enum {
        CHANNEL_COUNT = 1,
        SAMPLE_RATE = 44100,
    };
    if (wave.channel_count != CHANNEL_COUNT) {
        LOG_ERROR("channel count=%" PRIu16 ", expect=%d", wave.channel_count,
                  CHANNEL_COUNT);
        ok = false;
    }
    if (wave.sample_rate != SAMPLE_RATE) {
        LOG_ERROR("sample rate=%" PRIu32 ", expected=%d", wave.sample_rate,
                  SAMPLE_RATE);
        ok = false;
    }
    if (wave.bits_per_sample != info->bits_per_sample) {
        LOG_ERROR("bits=%" PRIu16 ", expect=%" PRIu16, wave.bits_per_sample,
                  info->bits_per_sample);
        ok = false;
    }
done:
    input_file_destroy(&file);
    return ok;
}

void test_wave(void) {
    bool failed = false;
    for (size_t i = 0; i < sizeof(kWAVEFiles) / sizeof(*kWAVEFiles); i++) {
        if (!test_wave1(&kWAVEFiles[i])) {
            failed = true;
        }
        log_context_clear();
    }
    if (failed) {
        test_failure_count++;
    }
}
