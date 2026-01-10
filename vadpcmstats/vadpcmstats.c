// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/vadpcm.h"
#include "common/audio.h"
#include "common/util.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void fail_pthread(int line, int errcode) __attribute__((noreturn));

static void fail_pthread(int line, int errcode) {
    log_error_errno(__FILE__, line, errcode, "pthread failure");
    abort();
}

#define CHECK_PTHREAD                  \
    do {                               \
        if (r != 0) {                  \
            fail_pthread(__LINE__, r); \
        }                              \
    } while (0)

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
    "  -j, --jobs n        Number of parallel jobs\n"
    "  -o, --output file   Write stats to CSV file\n"
    "  -p, --predictors n  Set the number of predictors to use (1..16, default "
    "4)\n";

static void collect_stats(struct vadpcm_params *params, const char *input_file,
                          struct vadpcm_stats *stats) {
    struct audio_data audio;
    int r = audio_read_pcm(&audio, input_file);
    if (r != 0) {
        goto error;
    }
    uint32_t vadpcm_frame_count =
        audio.padded_sample_count / kVADPCMFrameSampleCount;
    void *vadpcm_data = XMALLOC(vadpcm_frame_count, kVADPCMFrameByteSize);
    struct vadpcm_vector codebook[kVADPCMMaxPredictorCount];
    vadpcm_error err = vadpcm_encode(params, codebook, vadpcm_frame_count,
                                     vadpcm_data, audio.sample_data, stats);
    free(audio.sample_data);
    free(vadpcm_data);
    if (err != 0) {
        LOG_ERROR("encoding failed: %s; file=%s", vadpcm_error_name(err),
                  input_file);
        goto error;
    }
    return;

error:
    *stats = (struct vadpcm_stats){
        .signal_mean_square = -1.0,
        .error_mean_square = -1.0,
    };
    return;
}

struct stats_state {
    struct vadpcm_params params;
    char **input_files;
    struct vadpcm_stats *stats;

    pthread_mutex_t mutex;
    int index;
    int count;
};

static void *collect_stats_loop(void *arg) {
    struct stats_state *state = arg;
    int r;
    r = pthread_mutex_lock(&state->mutex);
    CHECK_PTHREAD;
    while (state->index < state->count) {
        int index = state->index++;
        r = pthread_mutex_unlock(&state->mutex);
        CHECK_PTHREAD;

        collect_stats(&state->params, state->input_files[index],
                      &state->stats[index]);

        r = pthread_mutex_lock(&state->mutex);
        CHECK_PTHREAD;
    }
    r = pthread_mutex_unlock(&state->mutex);
    CHECK_PTHREAD;
    return NULL;
}

int main(int argc, char **argv) {
    static const struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"jobs", required_argument, 0, 'j'},
        {"output", required_argument, 0, 'o'},
        {"predictors", required_argument, 0, 'p'},
        {0, 0, 0, 0},
    };
    int opt, option_index, jobs = 0;
    struct stats_state state;
    state.params = (struct vadpcm_params){
        .predictor_count = kDefaultPredictorCount,
    };
    const char *output_file = NULL;
    while ((opt = getopt_long(argc, argv, "ho:p:", long_options,
                              &option_index)) != -1) {
        switch (opt) {
        case 'h':
            fputs(HELP, stdout);
            return 0;
        case 'j': {
            char *end;
            unsigned long value = strtoul(optarg, &end, 10);
            if (value < 1 || INT_MAX < value) {
                LOG_ERROR("invalid value for --jobs");
                return 2;
            }
            jobs = value;
        } break;
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
            state.params.predictor_count = value;
        } break;
        default:
            return 2;
        }
    }
    char **input_files = argv + optind;
    int count = argc - optind;
    if (count == 0) {
        LOG_ERROR("no input files");
        return 2;
    }
    if (jobs == 0) {
        long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
        if (cpu_count <= 1) {
            jobs = 1;
        } else if (INT_MAX <= cpu_count) {
            jobs = INT_MAX;
        } else {
            jobs = cpu_count;
        }
    }
    if (jobs > count) {
        jobs = count;
    }
    state.input_files = input_files;
    state.stats = XMALLOC(count, sizeof(*state.stats));
    if (jobs <= 1) {
        for (int i = 0; i < count; i++) {
            collect_stats(&state.params, state.input_files[i], &state.stats[i]);
        }
    } else {
        state.index = 0;
        state.count = count;
        LOG_DEBUG("working in parallel; jobs=%d", jobs);
        int r;
        r = pthread_mutex_init(&state.mutex, NULL);
        CHECK_PTHREAD;
        pthread_t *threads = XMALLOC(jobs, sizeof(*threads));
        for (int i = 0; i < jobs; i++) {
            r = pthread_create(&threads[i], NULL, collect_stats_loop, &state);
            CHECK_PTHREAD;
        }
        for (int i = 0; i < jobs; i++) {
            void *value;
            r = pthread_join(threads[i], &value);
            CHECK_PTHREAD;
        }
        r = pthread_mutex_destroy(&state.mutex);
        CHECK_PTHREAD;
    }
    FILE *output = stdout;
    if (output_file != NULL) {
        output = fopen(output_file, "wb");
        if (output == NULL) {
            LOG_ERROR_ERRNO(errno, "open %s", output_file);
            return 1;
        }
    }
    fputs("file,signal_rms,error_rms\r\n", output);
    for (int i = 0; i < count; i++) {
        const char *input_file = input_files[i];
        const struct vadpcm_stats *stats = &state.stats[i];
        // FIXME: consider escaping, or an alternative format.
        if (stats->error_mean_square >= 0.0) {
            fprintf(output, "%s,%.5g,%.5g\r\n", input_file,
                    sqrt(stats->signal_mean_square),
                    sqrt(stats->error_mean_square));
        } else {
            fprintf(output, "%s,,\r\n", input_file);
        }
    }
    if (output_file != NULL) {
        fclose(output);
    }
    return 0;
}
