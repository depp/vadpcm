// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include <stdint.h>

// Extended precision floating-point number.
struct extended {
    uint16_t sign_exponent;
    uint16_t fraction[4];
};
