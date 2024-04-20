#ifndef HASH_SIMD_H
#define HASH_SIMD_H

// # SIMD/AVX2 temperature value hash calculation in `hash_temperature_using_simd()`
//
// All possible bit combinations of the last 5 bytes before `\n` (1 - any digit, a - any letter):
//
// pos:       -5          -4                -3                -2          -1
//
// [-111]:   0010.1101 (-)     0011.0*** (0-7)   0011.0*** (0-7)   0010.1110   0011.0*** (0-7)
//                             0011.100* (8-9)   0011.100* (8-9)   0010.1110   0011.100* (8-9)
// [;111]:   0011.1011 (;)     ----.---- (0-9)   ----.---- (0-9)   0010.1110   ----.---- (0-9)
// [;-11]:   0011.1011 (;)     0010.1101 (-)     ----.---- (0-9)   0010.1110   ----.---- (0-9)
// [a;11]:   ****.**** (any)   0011.1011 (;)     ----.---- (0-9)   0010.1110   ----.---- (0-9)
//
// filter:   ++++ xxxx         xxxx ++++         xxxx ++++         xxxx xxxx   xxxx ++++
// mask:     0xF0                   0x0F              0x0F              0x00        0x0F

// [aaaaaaaaaaa|;-1.2]
//
// chunk  orig    ... 0110.1111 0110.1111 | 0011.1011 0110.1111  ||  0011.0001 0010.1101 | 0011.0010 0010.1110
// chunk  masked  ... ----.---- ----.---- | 0011.---- ----.----  ||  ----.0001 ----.1101 | ----.0010 ----.----
// chunk2 shifted ... ----.---- ----.---- | 0011.---- ----.----  ||  ----.---- 0010.---- | ----.---- ----.----
// chunk2 shufled ... ----.---- ----.---- | ----.---- ----.----  ||  0011.---- 0010.---- | ----.---- ----.----
// chunk ^ chunk2 ... ----.---- ----.---- | 0011.---- ----.----  ||  0011.0001 0010.1011 | ----.0010 ----.----
// hash                                                              ++++ ++++ ++++ ++++

#include "defs.h"
#include "mappings.h"

#include <immintrin.h>

INLINE uint16_t extract_city_hash(char* name, size_t name_len) {
    register uint16_t hash;
    hash = *(uint16_t*) name;
    hash <<= 2;
    hash ^= *(uint16_t*) (name + 2);
    hash <<= 2;
    hash ^= *(uint16_t*) (name + name_len - 4);
    hash ^= *(uint16_t*) (name + name_len - 2);
    hash ^= (uint16_t)name_len;
    return hash;
}

INLINE int16_t extract_temperature_1__direct_calc(char *end) {
    char *c = end - 1;
    int16_t value = (*(c-2)-'0') * 10 + (*c - '0');
    c -=3;

    switch (*c) {
    case ';':
        break;
    case '-':
        value = -value;
        c --;
        break;
    default:
        value = (*c-'0') * 100 + value;
        c --;
        if (*c == '-') {
            value = -value;
            c --;
        }
        break;
    }

    return value;
}

INLINE int16_t extract_temperature_2__hash_lookup(char *start, char *end) {
    char *c = end - 1;
    uint16_t hash = (uint16_t)(*c);
    int shift = 4;
    for (c -= 2; c > start; c--, shift += 4)
        hash |= (uint16_t)(*c & 0x000F) << shift;
    return hash_to_value[hash];
}

INLINE int16_t extract_temperature_3__combined_lookups(char *end) {
    char *c = end - 1;
    int16_t value = 0;
    value += *c - '0';
    value += map10[(uint8_t)*(c-2)];
    value += map100[(uint8_t)*(c-3)];
    value *= sig[(uint8_t)*(c-3)];
    value *= sig[(uint8_t)*(c-4)];
    return value;
}


INLINE uint16_t hash_temperature_using_simd_swap(__m128i chunk) {
    const __m128i punch   = _mm_setr_epi8(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0xF0, 0x0F, 0x0F, 0x00, 0x0F);
    const __m128i shift   = _mm_setr_epi32(0, 0, 0, 20);
    const __m128i shuffle = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, 11, 12, -1, -1);

    chunk = _mm_and_si128(chunk, punch);           // CPI 0.33
    __m128i chunk2 = _mm_srlv_epi32(chunk, shift); // CPI 0.5
    chunk2 = _mm_shuffle_epi8(chunk2, shuffle);    // CPI 0.5
    chunk = _mm_or_si128(chunk, chunk2);           // CPI 0.33
    return _mm_extract_epi16(chunk, 6);            // CPI 1.0
}

INLINE int16_t extract_temperature_4__simd_swapped_hash_lookup(__m128i chunk) {
    // PERF: swapped_hash_to_value_simd - 78.838 bytes
    //       performe 5% worse than prev, despite `swapped_hash_to_value_simd` being 2x smaller than `hash_to_value_simd` (should be better for CPU cache)
    const uint16_t hash = hash_temperature_using_simd_swap(chunk);
    return swapped_hash_to_value_simd[hash];
}


INLINE uint16_t hash_temperature_using_simd(__m128i chunk) {
    // PERF: Seems slightly faster stable (on consecutive runs) when using 128, not 256 simd (less CPU heat?)
    // PERF: Moving these ouside reduces performance by 5%
    const __m128i punch   = _mm_setr_epi8(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0xF0, 0x0F, 0x0F, 0x00, 0x0F);
    const __m128i shift   = _mm_setr_epi32(0, 0, 0, 20);
    const __m128i shuffle = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, 12, 11, -1, -1);

    chunk = _mm_and_si128(chunk, punch);           // CPI 0.33
    __m128i chunk2 = _mm_srlv_epi32(chunk, shift); // CPI 0.5
    chunk2 = _mm_shuffle_epi8(chunk2, shuffle);    // CPI 0.5
    chunk = _mm_or_si128(chunk, chunk2);           // CPI 0.33
    return _mm_extract_epi16(chunk, 6);            // CPI 1.0
}

INLINE int16_t extract_temperature_5__simd_hash_lookup(__m128i chunk) {
    // hash_temperature_using_simd - 131.070 bytes
    const uint16_t hash = hash_temperature_using_simd(chunk);
    return hash_to_value_simd[hash];
}

#endif //HASH_SIMD_H
