// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/vadpcm.h"
#include "common/audio.h"
#include "common/format.h"
#include "common/getopt.h"
#include "common/util.h"
#include "vadpcm/commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off: let this be wide
static const char HELP[] =
    "Usage: vadpcm decode [options...] input_file output_file\n"
    "\n"
    "Decode a VADPCM-encoded audio file.\n"
    "\n"
    "Options:\n"
    "  --debug             Print debug messages\n"
    "  -h, --help          Show this help text\n"
    "  -q, --quiet         Only print warnings and errors\n";
// clang-format on

int cmd_decode(int argc, char **argv) {
    // Parse command-line.
    enum {
        opt_debug = 1,
    };
    static const struct option long_options[] = {
        {"debug", no_argument, 0, opt_debug},
        {"help", no_argument, 0, 'h'},
        {"quiet", no_argument, 0, 'q'},
        {0, 0, 0, 0},
    };
    int opt, option_index;
    optind = 2;
    while ((opt = getopt_long(argc, argv, "h", long_options, &option_index)) !=
           -1) {
        switch (opt) {
        case opt_debug:
            g_log_level = LEVEL_DEBUG;
            break;
        case 'h':
            fputs(HELP, stdout);
            return 0;
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
    file_format output_format = format_for_file(output_file);
    if (!check_format_vadpcm(input_file, input_format) ||
        !check_format_pcm_output(output_file, output_format)) {
        return 1;
    }

    // Read input.
    log_context("read", input_file);
    struct audio_vadpcm audio;
    int r = audio_read_vadpcm(&audio, input_file);
    if (r != 0) {
        return -1;
    }
    LOG_INFO("sample rate: %f", double_from_extended(&audio.meta.sample_rate));

    // Decode.
    log_context("decode", input_file);
    int16_t *pcm_data =
        XMALLOC(audio.meta.padded_sample_count, sizeof(*pcm_data));
    struct vadpcm_vector state;
    memset(&state, 0, sizeof(state));
    vadpcm_error err =
        vadpcm_decode(audio.codebook.predictor_count, audio.codebook.order,
                      audio.codebook.vector, &state,
                      audio.meta.padded_sample_count / kVADPCMFrameSampleCount,
                      pcm_data, audio.encoded_data);
    if (err != 0) {
        LOG_ERROR("decoding failed: %s", vadpcm_error_name(err));
        return 1;
    }

    // Write output.
    log_context("write", output_file);
    r = audio_write_pcm(
        &(struct audio_pcm){
            .meta = audio.meta,
            .sample_data = pcm_data,
        },
        output_file, output_format);
    if (r != 0) {
        return 1;
    }

    free(pcm_data);
    log_context_clear();
    return 0;
}
