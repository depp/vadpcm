// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "util/util.h"

#include <stdlib.h>

void *xmalloc(const char *file, int line, size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    size_t total;
    if (__builtin_mul_overflow(nmemb, size, &total)) {
        goto error;
    }
    void *ptr = malloc(total);
    if (ptr == NULL) {
        goto error;
    }
    return ptr;

error:
    log_error(file, line, "malloc failed; nmemb=%zu, size=%zu", nmemb, size);
    exit(1);
}
