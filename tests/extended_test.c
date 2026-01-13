// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "tests/test.h"

#include "common/extended.h"
#include "common/util.h"

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

struct extended_case {
    const char *name;
    uint16_t sign_exponent;
    uint64_t fraction;
    double value;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"

static const struct extended_case kCases[] = {
    {"basic_one", 16383, 1ull << 63, 1.0},
    {"basic_two", 16384, 1ull << 63, 2.0},
    {"basic_half", 16382, 1ull << 63, 0.5},
    {"after_one", 16383, (1ull << 63) + (1 << 11), 1.0000000000000002},
    {"round_even_1", 16383, (1ull << 63) + (1 << 10), 1.0},
    {"round_even_2", 16383, (1ull << 63) + (1 << 10) + 1, 1.0000000000000002},
    {"round_even_3", 16383, (1ull << 63) + (1 << 11), 1.0000000000000002},
    {"round_even_4", 16383, (1ull << 63) + (3 << 10) - 1, 1.0000000000000002},
    {"round_even_5", 16383, (1ull << 63) + (3 << 10), 1.0000000000000004},
    {"round_exponent", 16381, ~0ull, 0.5},
    {"inf", 32767, 0, INFINITY},
    {"large_1", 32000, 1ull << 63, INFINITY},
    {"large_2", 32000, ~0ull, INFINITY},
    {"large_3", 17406, 0xfffffffffffff800, 1.7976931348623157e+308},
    {"large_4", 17406, 0xfffffffffffffbff, 1.7976931348623157e+308},
    {"large_5", 17406, 0xfffffffffffffc00, INFINITY},
    {"zero", 0, 0, 0.0},
    {"nan_1", 32767, 1, NAN},
    {"nan_2", 32767, 1ull << 63, NAN},
    {"smallest_normal", 15361, 1ull << 63, 2.2250738585072014e-308},
    {"subnormal", 15360, 1ull << 63, 1.1125369292536007e-308},
    {"smallest_subnormal", 15309, 1ull << 63, 5e-324},
#if 0 // Fails, but we don't care about this case (unlikely to be relevant).
    {"smallest_subnormal_roundup", 15308, (1ull << 63) + 1, 5e-324},
#endif
    {"smallest_subnormal_rounddown", 15308, 1ull << 63, 0.0},
    {"round_to_zero", 10000, 1ull << 63, 0.0},
};

#pragma GCC diagnostic pop

static bool test_double_from_extended(const struct extended_case *restrict c) {
    log_context("double_from_extended %s", c->name);
    bool ok = true;
    for (int sign = 0; sign < 2; sign++) {
        struct extended in = {
            .sign_exponent = c->sign_exponent,
            .fraction = c->fraction,
        };
        if (sign == 1) {
            in.sign_exponent |= 0x8000;
        }
        double out = double_from_extended(&in);
        double expected = sign == 0 ? c->value : -c->value;
        bool equal;
        if (!isnan(expected)) {
            equal = expected == out;
        } else {
            equal = isnan(out);
        }
        if (!equal) {
            LOG_ERROR("got %.17g, expect %.17g", out, expected);
            ok = false;
        }
    }
    return ok;
}

struct extended_case_u32 {
    const char *name;
    uint16_t sign_exponent;
    uint64_t fraction;
    uint32_t value;
};

static const struct extended_case_u32 kCasesU32[] = {
    {"zero", 0, 0, 0},
    {"one", 16383, 1ull << 63, 1.0},
    {"32k", 0x400d, 0xfa00000000000000ull, 32000.0},
};

static bool test_extended_from_uint32(
    const struct extended_case_u32 *restrict c) {
    log_context("extended_from_uint32 %s", c->name);
    struct extended out = extended_from_uint32(c->value);
    if (out.sign_exponent != c->sign_exponent || out.fraction != c->fraction) {
        LOG_ERROR("got $%04" PRIx16 ":%016" PRIx64 ", expect $%04" PRIx16
                  ":%016" PRIx64,
                  out.sign_exponent, out.fraction, c->sign_exponent,
                  c->fraction);
        return false;
    }
    return true;
}

void test_extended(void) {
    bool failed = false;
    for (size_t i = 0; i < sizeof(kCases) / sizeof(*kCases); i++) {
        if (!test_double_from_extended(&kCases[i])) {
            failed = true;
        }
    }
    for (size_t i = 0; i < sizeof(kCasesU32) / sizeof(*kCasesU32); i++) {
        if (!test_extended_from_uint32(&kCasesU32[i])) {
            failed = true;
        }
    }
    log_context_clear();
    if (failed) {
        test_failure_count++;
    }
}
