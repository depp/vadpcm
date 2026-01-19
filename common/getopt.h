// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#if _WIN32

struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

enum {
    no_argument = 0,
    required_argument = 1,
};

int getopt_long(int argc, char **argv, const char *options,
                const struct option *long_options, int *idx);

extern char *optarg;
extern int optind;

#else
#include <getopt.h> // IWYU pragma: export
#endif
