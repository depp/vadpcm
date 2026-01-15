// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#if _WIN32
#include "common/getopt.h"

#include "common/util.h"

#include <string.h>

char *optarg;
int optind;

int getopt_long(int argc, char **argv, const char *options,
                const struct option *long_options, int *idx) {
    if (optind >= argc) {
        return -1;
    }
    char *arg = argv[optind];
    if (arg[0] != '-') {
        return -1;
    }
    int ch = arg[1];
    if (ch == '-') {
        if (arg[2] == '\0') {
            return -1;
        }
        char *name = arg + 2;
        char *value = strchr(name, '=');
        if (value != NULL) {
            *value++ = '\0';
        }
        for (const struct option *optr = long_options; optr->name != NULL;
             optr++) {
            if (strcmp(optr->name, name) == 0) {
                if (optr->has_arg) {
                    if (value != NULL) {
                        optarg = value;
                        optind++;
                    } else {
                        if (optind + 1 >= argc) {
                            LOG_ERROR("option requires an argument: %s", arg);
                            return '?';
                        }
                        optarg = argv[optind + 1];
                        optind += 2;
                    }
                } else if (value != NULL) {
                    LOG_ERROR("option does not take an argument: %s", arg);
                    return '?';
                } else {
                    optarg = NULL;
                    optind += 1;
                }
                return optr->val;
            }
        }
        LOG_ERROR("unknown option: %s", arg);
        return '?';
    }
    if (ch == '\0') {
        return -1;
    }
    if (arg[2] != '\0') {
        LOG_ERROR("unknown option: %s", arg);
        return '?';
    }
    char *ptr = strchr(options, ch);
    if (ptr == NULL) {
        LOG_ERROR("unknown option: %s", arg);
        return '?';
    }
    if (*(ptr + 1) == ':') {
        if (optind + 1 >= argc) {
            LOG_ERROR("option requires an argument: %s", arg);
            return '?';
        }
        optarg = argv[optind + 1];
        optind += 2;
    } else {
        optarg = NULL;
    }
    return ch;
}

#endif
