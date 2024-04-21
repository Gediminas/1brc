#include "printing.h"
#include "extraction.h"
#include <stdbool.h>
#include <string.h>
#include <immintrin.h> // avx, avx2, fma, avx-512

#define SWAP(a, b) a=a^b; b=a^b; a=b^a;

int main(int argc, char** argv) {
    const bool debg = (argc > 1) ? (strcmp(argv[1], "-d") == 0) : false;

    int16_t hash2value[UINT16_MAX];      //131.07 kB
    uint16_t same[UINT16_MAX];           //131.07 kB
    memset(hash2value, 0, UINT16_MAX*2);
    memset(same, 0, UINT16_MAX*2);

    for (int value = -999; value <= 999; value++) {
        const int v = abs(value);
        const char c = v % 10;
        const char b = v / 10 % 10;
        const char a = v / 100;

        char data[6] = "   . \n";
        uint8_t start;

        if (value < -99) {
            start = 0;
            data[0] = '-';
            data[1] = '0' + a;
        } else if (value < 0) {
            start = 1;
            data[0] = ';';
            data[1] = '-';
        } else if (value <= 99) {
            start = 2;
            data[0] = 'A';
            data[1] = ';';
        } else {
            start = 1;
            data[0] = ';';
            data[1] = '0' + a;
        }

        data[2] = '0' + b;
        data[4] = '0' + c;

        // NOTE: print all numbers
        // printf("%.5s  ", data); 

        // bool print = debg && (value == -123 || value == -12 ||  value == 12 || value == 123 || value == 4);
        // bool print = debg && (value == 4);
        bool print = false;
        if (print) printf("\n=== [%.5s] ====================\n", data);

        char data2[16] = "oooooooooooooooo";
        memcpy(&data2[11], data, 5);
        data2[start+10] = ';';

        for (uint8_t letter = 0; letter < UINT8_MAX; letter++) {
            const __m128i chunk = _mm_lddqu_si128((__m128i*) data2);

            if (print) {
                printf("[%.11s|%.5s]\n", data2, &data2[11]);
                print_bits_128(&chunk, false);
            }

            uint16_t hash = hash_temperature_using_simd(chunk);
            if (print) printf("HASH: %u => %d\n", hash, value);

            {
                char *end = &data2[15]+1;
                char *c = end - 1;
                int16_t value_check = (*(c-2)-'0') * 10 + (*c - '0');
                c -=3;
                if (*c != ';') {
                    if (*c == '-') {
                        value_check = -value_check;
                        c --;
                    }
                    else {
                        value_check = (*c-'0') * 100 + value_check;
                        c --;
                        if (*c == '-') {
                            value_check = -value_check;
                            c --;
                        }
                    }
                }
                if (value != value_check) {
                    printf("ERROR: %d<>%d\n", value, value_check);
                    return 1;
                }
            }

            hash2value[hash] = value;
            same[hash]++;

            if (0 <= value && value <= 99) {
                data2[11] = letter;
                continue;
            }

            break;
        }
    }

    if (debg) printf("\n============>\n");

    size_t total_collisions = 0;
    size_t total_empty = 0;
    // uint16_t max_hash_with_value = UINT16_MAX; //orig 63899,  swapped 39419;
    uint16_t max_hash_with_value = 39419;
    uint16_t max_hash_with_value_eval = 0;
    
    if (!debg) 
        printf("\nconst int16_t hash_to_value_simd[] = {");

    for (uint16_t hash = 0; hash < UINT16_MAX; hash++) {
        const int16_t value = hash2value[hash];
        const uint16_t count_same = same[hash];

        if (count_same == 0) {
            if (value != 0) {
                printf("ERROR zeroes\n");
                return 1;
            }
            total_empty ++;
        }
        else {
            if (0 <= value && value <= 99) {
                if (count_same != 14 && count_same != 16 && count_same != 17) {
                    printf("COLLISIONS !!!!!! %x:%d: %d      \n", hash, hash2value[hash], count_same);
                    total_collisions += count_same - 1;
                }
            }
            else {
                if (count_same > 1) {
                    printf("COLLISIONS !!!!!! %x:%d: %d      \n", hash, hash2value[hash], count_same);
                    total_collisions += count_same - 1;
                }
            }
        }

        // if (debg && value) 
        //     printf("%d=>%d ", hash, value);

        if (!debg) {
            if (hash % 50 == 0) {
                printf("\n  ");
            }
            printf("%d", value);
        }

        if (value != 0)
            max_hash_with_value_eval = hash;

        if (hash == max_hash_with_value)
            break;

        if (hash == UINT16_MAX-1)
            break;

        if (!debg)
            printf(",");
    }
    if (!debg) 
        printf("\n};");

    if (debg) {
        printf("\n");
        printf("max_mapping: %d\n", max_hash_with_value_eval);
        printf("TOTAL COLLISIONS: %ld\n", total_collisions);
        printf("TOTAL EMPTY: %ld\n", total_empty);
    }
        
    return 0;
}
