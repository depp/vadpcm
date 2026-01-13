// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/vadpcm.h"
#include "common/aiff.h"
#include "common/audio.h"
#include "common/format.h"
#include "common/util.h"
#include "vadpcm/commands.h"

#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    kDefaultPredictorCount = 4,
};

// clang-format off: let this be wide
static const char HELP[] =
    "Usage: vadpcm encode [options...] input_file output_file\n"
    "\n"
    "Encode an audio file using VADPCM.\n"
    "\n"
    "Options:\n"
    "  --debug             Print debug messages\n"
    "  -h, --help          Show this help text\n"
    "  -p, --predictors n  Set the number of predictors to use (1..16, default 4)\n"
    "  -q, --quiet         Only print warnings and errors\n";
// clang-format on

int cmd_encode(int argc, char **argv) {
    enum {
        opt_debug = 1,
    };
    static const struct option long_options[] = {
        {"debug", no_argument, 0, opt_debug},
        {"help", no_argument, 0, 'h'},
        {"predictors", required_argument, 0, 'p'},
        {"quiet", no_argument, 0, 'q'},
        {0, 0, 0, 0},
    };
    int opt, option_index;
    int predictor_count = kDefaultPredictorCount;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "hp:q", long_options,
                              &option_index)) != -1) {
        switch (opt) {
        case opt_debug:
            g_log_level = LEVEL_DEBUG;
            break;
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
        case 'q':
            g_log_level = LEVEL_QUIET;
            break;
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
    file_format input_format = format_for_file(input_file);
    if (input_format == kFormatUnknown) {
        log_context("read %s", input_file);
        LOG_ERROR("unknown file format (unrecognized extension)");
        return 1;
    }
    file_format output_format = format_for_file(output_file);
    if (output_format != kFormatAIFF && output_format != kFormatAIFC) {
        log_context("write %s", output_file);
        if (output_format == kFormatUnknown) {
            LOG_ERROR("unknown file format (unrecognized extension)");
        } else {
            LOG_ERROR("file format does not support VADPCM");
        }
        return 1;
    }
    if (g_log_level >= LEVEL_DEBUG) {
        LOG_DEBUG("input: %s", input_file);
        LOG_DEBUG("output: %s", output_file);
        LOG_DEBUG("predictor count: %d", predictor_count);
    }

    // Read input.
    log_context("read %s", input_file);
    struct audio_pcm audio;
    int r = audio_read_pcm(&audio, input_file, input_format);
    if (r != 0) {
        return 1;
    }
    if (g_log_level >= LEVEL_DEBUG) {
        LOG_INFO("sample rate: %f",
                 double_from_extended(&audio.meta.sample_rate));
    }

    // Encode.
    log_context("encode %s", input_file);
    uint32_t vadpcm_frame_count =
        audio.meta.padded_sample_count / kVADPCMFrameSampleCount;
    void *vadpcm_data = XMALLOC(vadpcm_frame_count, kVADPCMFrameByteSize);
    struct vadpcm_params params = {
        .predictor_count = predictor_count,
    };
    struct vadpcm_vector codebook[kVADPCMMaxPredictorCount];
    struct vadpcm_stats stats;
    vadpcm_error err = vadpcm_encode(&params, codebook, vadpcm_frame_count,
                                     vadpcm_data, audio.sample_data, &stats);
    if (err != 0) {
        LOG_ERROR("encoding failed: %s", vadpcm_error_name(err));
        return 1;
    }
    double signal_level = 10.0 * log10(stats.signal_mean_square);
    double error_level = 10.0 * log10(stats.error_mean_square);
    LOG_INFO("signal level: %.2f dB", signal_level);
    LOG_INFO("error level: %.2f dB", error_level);
    LOG_INFO("SNR: %.2f dB", signal_level - error_level);

    log_context("write %s", output_file);
    struct aiff_data aiff = {
        .version = kAIFFC,
        .version_timestamp = kAIFCVersion1,
        .num_channels = 1,
        // FIXME: use unpadded value?
        .num_sample_frames = audio.meta.padded_sample_count,
        .sample_size = 16,
        .sample_rate = audio.meta.sample_rate,
        .codec = kAIFFCodecVADPCM,
        .audio =
            {
                .ptr = vadpcm_data,
                .size = vadpcm_frame_count * kVADPCMFrameByteSize,
            },
        .codebook =
            {
                .order = kVADPCMEncodeOrder,
                .predictor_count = predictor_count,
                .vector = codebook,
            },
    };
    r = aiff_write(&aiff, output_file);
    if (r != 0) {
        return 1;
    }

    free(audio.sample_data);
    free(vadpcm_data);
    log_context_clear();
    return 0;
}
