// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "tests/test.h"

#include "common/extension.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct extension_case {
    const char *text;
    extension value;
};

static struct extension_case kCases[] = {
    {"dir/file.aif", kExtAIFF},
    {"dir/file.aiff", kExtAIFF},
    {"dir/file.aifc", kExtAIFC},
    {"dir/file.wav", kExtWAVE},
    {"dir/file.txt", kExtUnknown},
    {"dir/file.abc.aiff", kExtAIFF},
    {"", kExtUnknown},
    {"wav", kExtUnknown},
    {".wav", kExtUnknown},
    {"a.wav", kExtWAVE},
    {"a.WAV", kExtWAVE},
};

void test_extensions(void) {
    bool failed = false;
    for (size_t i = 0; i < sizeof(kCases) / sizeof(*kCases); i++) {
        extension out = extension_for_file(kCases[i].text);
        if (out != kCases[i].value) {
            fprintf(stderr, "extension_for_file(\"%s\") = %s; expect %s\n",
                    kCases[i].text, name_for_extension(out),
                    name_for_extension(kCases[i].value));
            failed = true;
        }
    }
    if (failed) {
        test_failure_count++;
    }
}
