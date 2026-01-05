// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/vadpcm.h"
#include "util/util.h"
#include "vadpcm/audio.h"

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    kDefaultPredictorCount = 4,
};

static const char HELP[] =
    "Usage: vadpcmstats [options] input_file...\n"
    "\n"
    "Collect codec statistics. Encodes one or more files and records the "
    "noise\n"
    "level for each.\n"
    "\n"
    "Options:\n"
    "  -h, --help          Show this help\n"
    "  -o, --output file   Write stats to CSV file\n"
    "  -p, --predictors n  Set the number of predictors to use (1..16, default "
    "4)\n";

static int collect_stats(const char *input_file,
                         const struct vadpcm_params *params, FILE *output) {
    struct audio_data audio;
    int r = audio_read_pcm(&audio, input_file);
    if (r != 0) {
        return -1;
    }
    uint32_t vadpcm_frame_count =
        audio.padded_sample_count / kVADPCMFrameSampleCount;
    void *vadpcm_data = XMALLOC(vadpcm_frame_count, kVADPCMFrameByteSize);
    struct vadpcm_vector codebook[kVADPCMMaxPredictorCount];
    struct vadpcm_stats stats;
    vadpcm_error err = vadpcm_encode(params, codebook, vadpcm_frame_count,
                                     vadpcm_data, audio.sample_data, &stats);
    if (err != 0) {
        LOG_ERROR("encoding failed: %s", vadpcm_error_name(err));
        return -1;
    }
    double signal_level = 10.0 * log10(stats.signal_mean_square);
    double error_level = 10.0 * log10(stats.error_mean_square);
    LOG_INFO("signal level: %.2f dB", signal_level);
    LOG_INFO("error level: %.2f dB", error_level);
    LOG_INFO("SNR: %.2f dB", signal_level - error_level);
    if (output != NULL) {
        // FIXME: consider escaping, or an alternative format.
        fprintf(output, "%s,%.5g,%.5g\r\n", input_file,
                sqrt(stats.signal_mean_square), sqrt(stats.error_mean_square));
    }
    return 0;
}

int main(int argc, char **argv) {
    static const struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"output", required_argument, 0, 'o'},
        {"predictors", required_argument, 0, 'p'},
        {0, 0, 0, 0},
    };
    int opt, option_index;
    struct vadpcm_params params = {
        .predictor_count = kDefaultPredictorCount,
    };
    const char *output_file = NULL;
    while ((opt = getopt_long(argc, argv, "ho:p:", long_options,
                              &option_index)) != -1) {
        switch (opt) {
        case 'h':
            fputs(HELP, stdout);
            return 0;
        case 'o':
            output_file = optarg;
            break;
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
            params.predictor_count = value;
        } break;
        default:
            return 2;
        }
    }
    if (argc == optind) {
        LOG_ERROR("no input files");
        return 2;
    }
    FILE *output = NULL;
    if (output_file != NULL) {
        output = fopen(output_file, "wb");
        if (output == NULL) {
            LOG_ERROR_ERRNO(errno, "open %s", output_file);
            return 1;
        }
        fputs("file,signal_rms,error_rms\r\n", output);
    }
    char **input_files = argv + optind;
    int input_file_count = argc - optind;
    for (int i = 0; i < input_file_count; i++) {
        int r = collect_stats(input_files[i], &params, output);
        if (r != 0) {
            return -1;
        }
    }
    (void)output_file;
    return 0;
}
