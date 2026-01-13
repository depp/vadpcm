// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include <stdint.h>

inline uint16_t read16be(const uint8_t *ptr) {
    return ((uint16_t)ptr[0] << 8) | (uint16_t)ptr[1];
}

inline uint32_t read32be(const uint8_t *ptr) {
    return ((uint32_t)ptr[0] << 24) | //
           ((uint32_t)ptr[1] << 16) | //
           ((uint32_t)ptr[2] << 8) |  //
           (uint32_t)ptr[3];
}

inline uint64_t read64be(const uint8_t *ptr) {
    return ((uint64_t)ptr[0] << 56) | //
           ((uint64_t)ptr[1] << 48) | //
           ((uint64_t)ptr[2] << 40) | //
           ((uint64_t)ptr[3] << 32) | //
           ((uint64_t)ptr[4] << 24) | //
           ((uint64_t)ptr[5] << 16) | //
           ((uint64_t)ptr[6] << 8) |  //
           (uint64_t)ptr[7];
}

inline void write16be(uint8_t *ptr, uint16_t value) {
    ptr[0] = value >> 8;
    ptr[1] = value;
}

inline void write32be(uint8_t *ptr, uint32_t value) {
    ptr[0] = value >> 24;
    ptr[1] = value >> 16;
    ptr[2] = value >> 8;
    ptr[3] = value;
}

inline void write64be(uint8_t *ptr, uint64_t value) {
    ptr[0] = value >> 56;
    ptr[1] = value >> 48;
    ptr[2] = value >> 40;
    ptr[3] = value >> 32;
    ptr[4] = value >> 24;
    ptr[5] = value >> 16;
    ptr[6] = value >> 8;
    ptr[7] = value;
}

inline uint16_t read16le(const uint8_t *ptr) {
    return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
}

inline uint32_t read32le(const uint8_t *ptr) {
    return (uint32_t)ptr[0] |         //
           ((uint32_t)ptr[1] << 8) |  //
           ((uint32_t)ptr[2] << 16) | //
           ((uint32_t)ptr[3] << 24);
}

inline void write16le(uint8_t *ptr, uint16_t value) {
    ptr[0] = value;
    ptr[1] = value >> 8;
}

inline void write32le(uint8_t *ptr, uint32_t value) {
    ptr[0] = value;
    ptr[1] = value >> 8;
    ptr[2] = value >> 16;
    ptr[3] = value >> 24;
}
