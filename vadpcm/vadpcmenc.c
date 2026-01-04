// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "aiff/aiff.h"
#include "codec/vadpcm.h"
#include "util/util.h"

#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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
    if (aiff.codec == kAIFFCodecVADPCM) {
        LOG_ERROR("input file is already encoded using VADPCM");
        return 1;
    }
    LOG_INFO("ok");
    return 0;
}
