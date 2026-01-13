// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "tests/test.h"

#include "common/format.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct extension_case {
    const char *text;
    file_format format;
};

static struct extension_case kCases[] = {
    {"dir/file.aif", kFormatAIFF},
    {"dir/file.aiff", kFormatAIFF},
    {"dir/file.aifc", kFormatAIFC},
    {"dir/file.wav", kFormatWAVE},
    {"dir/file.txt", kFormatUnknown},
    {"dir/file.abc.aiff", kFormatAIFF},
    {"", kFormatUnknown},
    {"wav", kFormatUnknown},
    {".wav", kFormatUnknown},
    {"a.wav", kFormatWAVE},
    {"a.WAV", kFormatWAVE},
};

void test_extensions(void) {
    bool failed = false;
    for (size_t i = 0; i < sizeof(kCases) / sizeof(*kCases); i++) {
        file_format out = format_for_file(kCases[i].text);
        if (out != kCases[i].format) {
            fprintf(stderr, "extension_for_file(\"%s\") = %s; expect %s\n",
                    kCases[i].text, name_for_format(out),
                    name_for_format(kCases[i].format));
            failed = true;
        }
    }
    if (failed) {
        test_failure_count++;
    }
}
