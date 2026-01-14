// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include <stdbool.h>

typedef enum {
    kFormatUnknown,
    kFormatAIFF,
    kFormatAIFC,
    kFormatWAVE,
} file_format;

// Figure out the filetype for a file, if we can.
file_format format_for_file(const char *filename);

// Return the name for a file extension.
const char *name_for_format(file_format fmt);

// Check that a format is valid for PCM input.
bool check_format_pcm_input(const char *name, file_format fmt);

// Check that a format is valid for PCM output.
bool check_format_pcm_output(const char *name, file_format fmt);

// Check that a format is valid for VADPCM.
bool check_format_vadpcm(const char *name, file_format fmt);
