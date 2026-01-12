// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/vadpcm.h"
#include "common/aiff.h"
#include "common/audio.h"
#include "common/util.h"
#include "vadpcm/commands.h"

#include <getopt.h>
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

static void swap16_inplace(int16_t *ptr, size_t size);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
static void swap16_inplace(int16_t *ptr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ptr[i] = __builtin_bswap16(ptr[i]);
    }
}
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static void swap16_inplace(int16_t *ptr, size_t size) {
    (void)ptr;
    (void)size;
}
#else
#error "Unknown byte order"
#endif

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

    // Main command.
    struct audio_vadpcm audio;
    int r = audio_read_vadpcm(&audio, input_file);
    if (r != 0) {
        return -1;
    }
    LOG_INFO("sample rate: %f", double_from_extended(&audio.meta.sample_rate));
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
    swap16_inplace(pcm_data, audio.meta.padded_sample_count);
    struct aiff_data aiff = {
        .version = kAIFF,
        .num_channels = 1,
        .num_sample_frames = audio.meta.original_sample_count,
        .sample_size = 16,
        .sample_rate = audio.meta.sample_rate,
        .codec = kAIFFCodecPCM,
        .audio =
            {
                .ptr = pcm_data,
                .size = sizeof(*pcm_data) * audio.meta.original_sample_count,
            },
    };
    r = aiff_write(&aiff, output_file);
    if (r != 0) {
        return 1;
    }

    free(pcm_data);
    return 0;
}
