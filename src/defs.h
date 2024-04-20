#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_BYTES (64L * 1024L * 1024L)
#define MIN_RECORD_BYTES 7
#define MIN_RECORD_WITH_SEP_BYTES 8
#define MAX_RECORD_BYTES 32
#define ALIGNMENT 64
#define DISK_CLUSTER 4096
#define RING_BUFFER_WRAPUP_LEN DISK_CLUSTER

// 16 gives better results than 64 and 32
#define STRUCT_ALIGN(x) __attribute__((aligned(x)))

#define INLINE __attribute__((always_inline)) static inline


/////////////////////////////
// RING

//core20 - 759-...ms
// #define RING_ENTRIES   1
// #define READ_BLOCK_LEN (4096 * 12)

//core20 - 756-...ms
#define RING_ENTRIES   2
#define RING_EXTRA_ENTRIES   2
#define READ_BLOCK_LEN (12 * DISK_CLUSTER)

//core20 - 765-...ms
// #define RING_ENTRIES   4
// #define READ_BLOCK_LEN (4096 * 12)

//core20 - 780-...ms
// #define RING_ENTRIES   8
// #define READ_BLOCK_LEN (1024 * 16)

/////////////////////////////
// STATIONS

#define MAX_STATION_NAME 32
#define MAX_STATIONS     512

// // #1
// #define STATION_HASH_MOD (1024 * 1024)
// #define STATION_HASH_MOD (32 * 1024 + 13)

//ok
//#1 MAX
#define STATION_HASH_MOD (64 * 1024)  
// #define STATION_HASH_MOD (33 * 1024)
// #define STATION_HASH_MOD (25 * 1024)

//#2
// #define STATION_HASH_MOD (16 * 1024)

// // #2
// // #define STATION_HASH_MOD (32 * 1024 + 13)

// #define STATION_HASH_MOD (64 * 1024 + 13)
// #define STATION_HASH_MOD (16 * 1024 + 1111)

#define LIKELY(x) __builtin_expect((x),1)
#define UNLIKELY(x) __builtin_expect((x),0)

/////////////////////////////
// TESTING

#define ___EXPECT(expr, msg)                    \
    if (UNLIKELY(!(expr))) {                    \
       if (errno)                               \
           perror(msg);                         \
       else                                     \
           fprintf(stderr, "ERROR: %s\n", msg); \
       exit(EXIT_FAILURE);                      \
    }

#ifdef DEBUG
    #define ___ASSERT(expr, msg) ___EXPECT(expr, msg)
#else
    #define ___ASSERT(expr, msg)
#endif

#endif //DEFS_H
