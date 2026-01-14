// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "common/binary.h" // IWYU pragma: keep

uint16_t read16be(const uint8_t *ptr);
uint32_t read32be(const uint8_t *ptr);
uint64_t read64be(const uint8_t *ptr);

void write16be(uint8_t *ptr, uint16_t value);
void write32be(uint8_t *ptr, uint32_t value);
void write64be(uint8_t *ptr, uint64_t value);

uint16_t read16le(const uint8_t *ptr);
uint32_t read32le(const uint8_t *ptr);

void write16le(uint8_t *ptr, uint16_t value);
void write32le(uint8_t *ptr, uint32_t value);

void swap16le(void *dest, const void *src, size_t size);
void swap16le_inplace(void *ptr, size_t size);
void swap16be(void *dest, const void *src, size_t size);
void swap16be_inplace(void *ptr, size_t size);

void swap16(void *dest, const void *src, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ((uint16_t *)dest)[i] = __builtin_bswap16(((const uint16_t *)src)[i]);
    }
}
