#ifndef PRINTING_H
#define PRINTING_H

#include "defs.h"
#include <stdbool.h>
#include <immintrin.h>

INLINE void print_bits_16(uint16_t value) {
    char buf[20] = "___________________";
    for (int i = 15; i >= 0; i--) {
        int bit = (value >> i) & 1;
        int pos = 15-i;
        buf[pos+pos/4] = bit ? '1' : '0';
    }
    buf[4] = '.';
    buf[9] = ' ';
    buf[14] = '.';
        for (int i = 0; i <16; i += 5)
        if (buf[i+0] == '0' && buf[i+1] == '0' && buf[i+2] == '0' && buf[i+3] == '0') {
            buf[i+0] = buf[i+1] = buf[i+2] = buf[i+3] = '-';
        }
    printf("%.19s", buf);
}

INLINE void print_bits_8(char value) {
    for (int i = 7; i >= 0; i--) {
        int bit = (value >> i) & 1;
        printf("%d", bit);
        if (i % 8 == 0) {
        }
        else if (i % 4 == 0) {
            printf(".");
        }
    }
    // printf("\n");
}

INLINE void print_bits_128(const __m128i *chunk, bool full) {
    char data2[16];
    _mm_storeu_si128((__m128i*) data2, *chunk);
    if (full) {
        print_bits_16(*((uint16_t*)&data2[0])); printf(" ");
        print_bits_16(*((uint16_t*)&data2[2])); printf(", ");
        print_bits_16(*((uint16_t*)&data2[4])); printf(" ");
        print_bits_16(*((uint16_t*)&data2[6])); printf("|| ");
    }
    printf("... ");
    print_bits_16(*((uint16_t*)&data2[8+0])); printf(" | ");
    print_bits_16(*((uint16_t*)&data2[8+2])); printf("  ||  ");
    print_bits_16(*((uint16_t*)&data2[8+4])); printf(" | ");
    print_bits_16(*((uint16_t*)&data2[8+6]));
    printf("\n");
}

#endif //PRINTING_H
