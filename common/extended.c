// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/extended.h"

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
    double value = scalbn((double)extended->fraction, exponent - 16383 - 63);
    return sign ? -value : value;
}

struct extended extended_from_uint32(uint32_t value) {
    if (value == 0) {
        return (struct extended){
            .sign_exponent = 0,
            .fraction = 0,
        };
    }
    union {
        double f;
        uint64_t u;
    } u;
    u.f = (double)value;
    return (struct extended){
        .sign_exponent = (u.u >> 52) + 16384 - 1024,
        .fraction = (u.u << 11) | (1ull << 63),
    };
}
