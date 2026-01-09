// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/binary.h"

uint16_t read16be(const uint8_t *ptr);
uint32_t read32be(const uint8_t *ptr);
uint64_t read64be(const uint8_t *ptr);

void write16be(uint8_t *ptr, uint16_t value);
void write32be(uint8_t *ptr, uint32_t value);
void write64be(uint8_t *ptr, uint64_t value);
