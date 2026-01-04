// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "aiff/aiff.h"
#include "codec/vadpcm.h"
#include "util/util.h"

#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of samples in an input file. This prevents the number of
// frames from overflowing a uint32_t after padding.
#define MAX_INPUT_LENGTH (~((uint32_t)(kVADPCMFrameSampleCount - 1)))

enum {
    kDefaultPredictorCount = 4,
};

static const char HELP[] =
    "Usage: vadpcmenc [options] input_file output_file\n"
    "\n"
    "Encode an audio file using VADPCM.\n"
    "\n"
    "Options:\n"
    "  -h, --help          Show this help\n"
    "  -p, --predictors n  Set the number of predictors to use (1..16, default "
    "4)\n";

// Copy 16-bit big-endian samples into the destination.
static void copy_samples(int16_t *dest, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] = ((uint16_t)src[2 * i] << 8) | (uint16_t)src[2 * i + 1];
    }
}

int main(int argc, char **argv) {
    static const struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"predictors", required_argument, 0, 'p'},
        {0, 0, 0, 0},
    };
    int opt, option_index;
    int predictor_count = kDefaultPredictorCount;
    while ((opt = getopt_long(argc, argv, "hp:", long_options,
                              &option_index)) != -1) {
        switch (opt) {
        case 'h':
            fputs(HELP, stdout);
            return 0;
        case 'p': {
            char *end;
            unsigned long value = strtoul(optarg, &end, 10);
            if (*optarg == '\0' || *end != '\0') {
                LOG_ERROR("invalid value for --predictors");
                return 2;
            }
            if (value < 1 || kVADPCMMaxPredictorCount < value) {
                LOG_ERROR("predictor count must be in the range 1..16");
                return 2;
            }
            predictor_count = value;
        } break;
        default:
            return 2;
        }
    }
    if (argc - optind < 2) {
        LOG_ERROR("not enough arguments, expected input file and output file");
        return 2;
    }
    if (argc - optind > 2) {
        LOG_ERROR("too many arguments, expected input file and output file");
        return 2;
    }
    const char *input_file = argv[optind];
    const char *output_file = argv[optind + 1];
    LOG_DEBUG("input: %s", input_file);
    LOG_DEBUG("output: %s", output_file);
    LOG_DEBUG("predictor count: %d", predictor_count);
    struct input_file input;
    int r = input_file_read(&input, input_file);
    if (r != 0) {
        return 1;
    }
    struct aiff_data aiff;
    r = aiff_parse(&aiff, input.data, input.size);
    if (r != 0) {
        return 1;
    }
    LOG_DEBUG("read input file");
    if (aiff.codec == kAIFFCodecVADPCM) {
        LOG_ERROR("input file is already encoded using VADPCM");
        return 1;
    }
    if (aiff.num_channels != 1) {
        LOG_ERROR("only mono files are supported; channels=%" PRIu32,
                  aiff.num_channels);
        return 1;
    }
    if (aiff.sample_size != 16) {
        LOG_ERROR("only 16-bit samples are supported; bits=%" PRIu32,
                  aiff.sample_size);
        return 1;
    }
    if (aiff.num_sample_frames > MAX_INPUT_LENGTH) {
        LOG_ERROR("input file is too long; length=%" PRIu32
                  ", maximum=%" PRIu32,
                  aiff.num_sample_frames, MAX_INPUT_LENGTH);
        return 1;
    }

    // PCM and VADPCM frame counts.
    uint32_t pcm_frame_count = aiff.num_sample_frames;
    uint32_t vadpcm_frame_count =
        pcm_frame_count / kVADPCMFrameSampleCount +
        ((pcm_frame_count & (kVADPCMFrameSampleCount - 1)) != 0);
    // PCM data is padded to a multiple of the VADPCM frame size. No overflow
    // due to MAX_INPUT_LENGTH check.
    uint32_t padded_frame_count = pcm_frame_count * kVADPCMFrameSampleCount;
    int16_t *pcm_data = XMALLOC(padded_frame_count, sizeof(*pcm_data));
    copy_samples(pcm_data, aiff.audio_ptr, pcm_frame_count);
    memset(pcm_data + pcm_frame_count, 0,
           sizeof(int16_t) * (padded_frame_count - pcm_frame_count));

    void *vadpcm_data = XMALLOC(vadpcm_frame_count, kVADPCMFrameByteSize);
    struct vadpcm_params params = {
        .predictor_count = predictor_count,
    };
    struct vadpcm_vector codebook[kVADPCMMaxPredictorCount];
    struct vadpcm_stats stats;
    vadpcm_error err = vadpcm_encode(&params, codebook, vadpcm_frame_count,
                                     vadpcm_data, pcm_data, &stats);
    if (err != 0) {
        LOG_ERROR("encoding failed: %s", vadpcm_error_name(err));
        return 1;
    }
    double signal_level = 10.0 * log10(stats.signal_mean_square);
    double error_level = 10.0 * log10(stats.error_mean_square);
    LOG_INFO("signal level: %.2f dB", signal_level);
    LOG_INFO("error level: %.2f dB", error_level);
    LOG_INFO("SNR: %.2f dB", signal_level - error_level);

    LOG_INFO("ok");
    return 0;
}
