// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/audio.h"

#include "common/aiff.h"
#include "common/binary.h"
#include "common/extended.h"
#include "common/util.h"
#include "common/wave.h"

static int audio_write_aiff(struct audio_pcm *restrict audio,
                            const char *filename, aiff_version version) {
    swap16be_inplace(audio->sample_data, audio->meta.padded_sample_count);
    struct aiff_data aiff = {
        .version = version,
        .version_timestamp = kAIFCVersion1,
        .num_channels = 1,
        .num_sample_frames = audio->meta.original_sample_count,
        .sample_size = 16,
        .sample_rate = audio->meta.sample_rate,
        .codec = kAIFFCodecPCM,
        .audio =
            {
                .ptr = audio->sample_data,
                .size = sizeof(*audio->sample_data) *
                        audio->meta.original_sample_count,
            },
    };
    return aiff_write(&aiff, filename);
}

static int audio_write_wave(struct audio_pcm *restrict audio,
                            const char *filename) {
    uint32_t sample_rate = uint32_from_extended(&audio->meta.sample_rate);
    swap16le_inplace(audio->sample_data, audio->meta.padded_sample_count);
    struct wave_data wave = {
        .codec = kWAVECodecPCM,
        .channel_count = 1,
        .sample_rate = sample_rate,
        .bytes_per_second = 2 * sample_rate,
        .block_align = 2,
        .bits_per_sample = 16,
        .audio =
            {
                .ptr = audio->sample_data,
                .size = sizeof(*audio->sample_data) *
                        audio->meta.original_sample_count,
            },
    };
    return wave_write(&wave, filename);
}

int audio_write_pcm(struct audio_pcm *restrict audio, const char *filename,
                    file_format format) {
    switch (format) {
    case kFormatAIFF:
        return audio_write_aiff(audio, filename, kAIFF);
    case kFormatAIFC:
        return audio_write_aiff(audio, filename, kAIFFC);
    case kFormatWAVE:
        return audio_write_wave(audio, filename);
        return -1;
    default:
        LOG_ERROR("unknown format");
        return -1;
    }
    return 0;
}
