// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/util.h"
#include "vadpcm/commands.h"

#include <stdio.h>
#include <string.h>

// clang-format off: let this be wide
static const char HELP[] =
    "vadpcm - VADPCM audio encoder and decoder\n"
    "\n"
    "VADPCM is a lossy audio codec which encodes data at a fixed rate of 9 bytes\n"
    "per 16 samples, or 4.5 bits per sample. It is most commonly used for Nintendo\n"
    "64 games, including games made with LibDragon and games made with the original\n"
    "console SDK. The audio quality of VADPCM is generally lower than more modern\n"
    "codecs.\n"
    "\n"
    "Usage: vadpcm [-v | --version] [-h | --help] command [arg...]\n"
    "\n"
    "Options:\n"
    "  -h, --help     Show this help text\n"
    "  -v, --version  Show the version of this software\n"
    "\n"
    "Commands:\n"
    "  encode  Encode an audio file using VADPCM.\n"
    "  help    Show help information.\n";
// clang-format on

static void cmd_help(void) {
    fputs(HELP, stdout);
}

static void cmd_version(void) {
    fputs("vadpcm version 0.1\n", stdout);
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        cmd_help();
        return 2;
    }
    char *arg = argv[1];
    if (arg[0] == '-') {
        switch (arg[1]) {
        case '-': {
            const char *name = arg + 2;
            if (strcmp(name, "help") == 0) {
                cmd_help();
                return 0;
            }
            if (strcmp(name, "version") == 0) {
                cmd_version();
                return 0;
            }
        } break;
        case 'h':
            cmd_help();
            return 0;
        case 'v':
            cmd_version();
            return 0;
        }
        LOG_ERROR("unknown option: '-%c'", arg[1]);
        return 2;
    }
    if (strcmp(arg, "encode") == 0) {
        return cmd_encode(argc, argv);
    }
    if (strcmp(arg, "help") == 0) {
        cmd_help();
        return 0;
    }
    LOG_ERROR("unknown command: '%s'", arg);
    return 2;
}
