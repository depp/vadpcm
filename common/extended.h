// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include <stdint.h>

// Extended precision floating-point number.
struct extended {
    uint16_t sign_exponent;
    uint64_t fraction;
};

// Convert an extended float to a double.
double double_from_extended(const struct extended *extended);

// Convert a uint32_t to extended.
struct extended extended_from_uint32(uint32_t value);
