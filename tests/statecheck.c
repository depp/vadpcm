// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/autocorr.h"
#include "codec/encode.h"
#include "codec/predictor.h"
#include "codec/vadpcm.h"
#include "common/audio.h"
#include "common/format.h"
#include "common/getopt.h"
#include "common/util.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off: let this be wide
static const char HELP[] =
    "Usage: statecheck [options...] input_file...\n"
    "\n"
    "Verify that encoder and decoder state match.\n"
    "\n"
    "Options:\n"
    "  -h, --help          Show this help text\n"
    "  -p, --predictors n  Set the number of predictors to use (1..16, default 4)\n";
// clang-format on

static void dump_state(const struct vadpcm_encoder_state *state,
                       const struct vadpcm_vector *codebook,
                       const int16_t *input, const uint8_t *vadpcm,
                       const int16_t *decoded) {
    fputs("  {\n", stderr);

    fputs("    .input = {", stderr);
    for (int i = 0; i < kVADPCMFrameSampleCount; i++) {
        if (i != 0) {
            fputs(", ", stderr);
        }
        fprintf(stderr, "%d", input[i]);
    }
    fputs("},\n", stderr);

    fputs("    .predictor = {\n", stderr);
    for (int i = 0; i < kVADPCMEncodeOrder; i++) {
        fputs("      {{", stderr);
        for (int j = 0; j < kVADPCMVectorSampleCount; j++) {
            if (j != 0) {
                fputs(", ", stderr);
            }
            fprintf(stderr, "%d", codebook[i].v[j]);
        }
        fputs("}},\n", stderr);
    }
    fputs("    },\n", stderr);

    fprintf(stderr, "    .state = {{%d, %d}, 0x%08" PRIx32 "},\n",
            state->data[0], state->data[1], state->rng);
    fputs("  }\n", stderr);

    fputs("  output:", stderr);
    for (int i = 0; i < kVADPCMFrameByteSize; i++) {
        fprintf(stderr, " %02x", vadpcm[i]);
    }
    fputc('\n', stderr);

    fputs("  decoded:", stderr);
    for (int i = 0; i < kVADPCMFrameSampleCount; i++) {
        fprintf(stderr, " %d", decoded[i]);
    }
    fputc('\n', stderr);
}

static int check_file(const char *file, int predictor_count) {
    bool ok = false;
    file_format input_format = format_for_file(file);
    if (!check_format_pcm_output(file, input_format)) {
        return false;
    }
    struct audio_pcm pcm;
    log_context("read", file);
    int r = audio_read_pcm(&pcm, file, input_format);
    if (r != 0) {
        return false;
    }
    log_context("check", file);
    size_t frame_count = pcm.meta.padded_sample_count / kVADPCMFrameSampleCount;
    float (*corr)[6] = XMALLOC(frame_count, sizeof(*corr));
    uint8_t *predictors = XMALLOC(frame_count, sizeof(*predictors));
    vadpcm_autocorr(frame_count, corr, pcm.sample_data);
    vadpcm_error err = vadpcm_assign_predictors(frame_count, predictor_count,
                                                corr, predictors);
    if (err != 0) {
        LOG_ERROR("could not assign predictors: %s", vadpcm_error_name(err));
        goto done1;
    }
    struct vadpcm_vector
        codebook[kVADPCMEncodeOrder * kVADPCMMaxPredictorCount];
    vadpcm_make_codebook(frame_count, predictor_count, corr, predictors,
                         codebook);
    struct vadpcm_stats stats_buf;
    struct vadpcm_encoder_state encoder_state;
    struct vadpcm_vector decoder_state;

    // Encode as single large block.
    memset(&encoder_state, 0, sizeof(encoder_state));
    uint8_t *vadpcm_full =
        XMALLOC(frame_count * kVADPCMFrameByteSize, sizeof(*vadpcm_full));
    vadpcm_encode_data(frame_count, vadpcm_full, pcm.sample_data, predictors,
                       codebook, &stats_buf, &encoder_state);
    int16_t *decoded_full =
        XMALLOC(frame_count * kVADPCMFrameSampleCount, sizeof(*decoded_full));
    memset(&decoder_state, 0, sizeof(decoder_state));
    err = vadpcm_decode(predictor_count, kVADPCMEncodeOrder, codebook,
                        &decoder_state, frame_count, decoded_full, vadpcm_full);

    // Encode many times, check state.
    memset(&encoder_state, 0, sizeof(encoder_state));
    memset(&decoder_state, 0, sizeof(decoder_state));
    for (size_t frame = 0; frame < frame_count; frame++) {
        struct vadpcm_encoder_state prev_state = encoder_state;
        uint8_t vadpcm[kVADPCMFrameByteSize];
        int16_t decoded[kVADPCMFrameSampleCount];
        vadpcm_encode_data(
            1, vadpcm, pcm.sample_data + frame * kVADPCMFrameSampleCount,
            predictors + frame, codebook, &stats_buf, &encoder_state);
        if (memcmp(vadpcm, vadpcm_full + frame * kVADPCMFrameByteSize,
                   sizeof(vadpcm)) != 0) {
            LOG_ERROR("encode mismatch; frame=%zu", frame);
            goto bad_frame;
        }
        err = vadpcm_decode(predictor_count, kVADPCMEncodeOrder, codebook,
                            &decoder_state, 1, decoded, vadpcm);
        if (err != 0) {
            LOG_ERROR("could not decode: frame=%zu; %s", frame,
                      vadpcm_error_name(err));
            goto bad_frame;
        }
        if (memcmp(decoded, decoded_full + frame * kVADPCMFrameSampleCount,
                   sizeof(decoded)) != 0) {
            LOG_ERROR("decode mismatch; frame=%zu", frame);
            goto bad_frame;
        }
        if (encoder_state.data[0] != decoder_state.v[6] ||
            encoder_state.data[1] != decoder_state.v[7]) {
            LOG_ERROR("state mismatch: frame=%zu", frame);
            fprintf(stderr, "  encoder state: %d %d\n", encoder_state.data[0],
                    encoder_state.data[1]);
            fprintf(stderr, "  decoder state: %d %d\n", decoder_state.v[6],
                    decoder_state.v[7]);
            goto bad_frame;
        }
        continue;
    bad_frame:
        dump_state(
            &prev_state, codebook + kVADPCMEncodeOrder * predictors[frame],
            pcm.sample_data + frame * kVADPCMFrameSampleCount, vadpcm, decoded);
        ok = false;
        goto done2;
    }
    ok = true;
done2:
    free(decoded_full);
    free(vadpcm_full);
done1:
    free(predictors);
    free(corr);
    audio_pcm_destroy(&pcm); // TODO: function for this?
    return ok;
}

int main(int argc, char **argv) {
    int opt, option_index;
    int predictor_count = 4;
    static const struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"predictors", required_argument, 0, 'p'},
        {0, 0, 0, 0},
    };
    g_log_level = LEVEL_DEBUG;
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
                LOG_ERROR("predictor count must be in the range 1..%d",
                          kVADPCMMaxPredictorCount);
                return 2;
            }
            predictor_count = value;
        } break;
        default:
            return 2;
        }
    }
    if (argc - optind < 1) {
        LOG_ERROR("expected input files");
        return 2;
    }
    int failures = 0;
    for (int i = optind; i < argc; i++) {
        if (!check_file(argv[i], predictor_count)) {
            failures++;
        }
    }
    if (failures > 0) {
        LOG_ERROR("failures: %d", failures);
        return 1;
    }
    return 0;
}
