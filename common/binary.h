// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ============================================================================
// Scalar Operations
// ============================================================================

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

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

#if _MSC_VER
#pragma warning(pop)
#endif

// ============================================================================
// Array Operations
// ============================================================================

// Byte-swap 16-bit values. The number of 16-bit values is given by 'size'.
void swap16(void *dest, const void *src, size_t size);

// Swap 16-bit values between little endian and native.
inline void swap16le(void *dest, const void *src, size_t size);

// Swap 16-bit values between little endian and native.
inline void swap16le_inplace(void *ptr, size_t size);

// Swap 16-bit values between big endian and native.
inline void swap16be(void *dest, const void *src, size_t size);

// Swap 16-bit values between big endian and native.
inline void swap16be_inplace(void *ptr, size_t size);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
inline void swap16le(void *dest, const void *src, size_t size) {
    memcpy(dest, src, 2 * size);
}
inline void swap16le_inplace(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
}
inline void swap16be(void *dest, const void *src, size_t size) {
    swap16(dest, src, size);
}
inline void swap16be_inplace(void *ptr, size_t size) {
    swap16(ptr, ptr, size);
}
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
inline void swap16le(void *dest, const void *src, size_t size) {
    swap16(dest, src, size);
}
inline void swap16le_inplace(void *ptr, size_t size) {
    swap16(ptr, ptr, size);
}
inline void swap16be(void *dest, const void *src, size_t size) {
    memcpy(dest, src, 2 * size);
}
inline void swap16be_inplace(void *ptr, size_t size) {
    (void)ptr;
    (void)size;
}
#else
#error "Unknown byte order"
#endif
