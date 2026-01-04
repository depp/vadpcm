// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/vadpcm.h"

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
                fputs("Error: invalid value for --predictors\n", stderr);
                return 2;
            }
            if (value < 1) {
                fputs("Error: predictor count may not be zero\n", stderr);
                return 2;
            }
            if (kVADPCMMaxPredictorCount < value) {
                fputs("Error: predictor count may not be larger than 16\n",
                      stderr);
                return 2;
            }
            predictor_count = value;
        } break;
        default:
            return 2;
        }
    }
    if (argc - optind != 2) {
        fputs("Error: expected input file and output file\n", stderr);
        return 2;
    }
    const char *input_file = argv[optind];
    const char *output_file = argv[optind];
    fprintf(stderr,
            "input: %s\n"
            "output: %s\n"
            "predictor count: %d\n",
            input_file, output_file, predictor_count);
    return 0;
}
