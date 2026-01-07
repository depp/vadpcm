// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "util/extended.h"

#include <math.h>

double double_from_extended(const struct extended *extended) {
    int exponent = extended->sign_exponent & 0x7fff;
    int sign = extended->sign_exponent & 0x8000;

    // Non-finite.
    if (exponent == 0x7fff) {
        if (extended->fraction == 0) {
            return sign ? -(double)INFINITY : (double)INFINITY;
        }
        return (double)NAN;
    }

    // Zero. We don't care about signed zeroes.
    if (extended->fraction == 0) {
        return 0.0;
    }
    return scalbn((double)extended->fraction, exponent - 16383 - 63);
}
