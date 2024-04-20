#ifndef PROCESS_MMAP_H
#define PROCESS_MMAP_H

#include "defs.h"
#include "process_common.h"
#include <sys/mman.h>

INLINE char *init_mmap(int fd, size_t fsize) {
     char *buf = (char*) mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
     ___EXPECT(buf != MAP_FAILED, "mmap-init");

     madvise(buf, fsize, MADV_SEQUENTIAL);
     madvise(buf, fsize, MADV_HUGEPAGE);
     madvise(buf, fsize, MADV_WILLNEED);
     madvise(buf, fsize, MADV_HWPOISON); //maybe faster 2-3%
     // madvise(buf, fsize, MADV_DONTNEED_LOCKED); //same
     // madvise(buf, fsize, MADV_POPULATE_READ);  //35% slower
     return buf;
 }

INLINE void *process_fsegment_mmap(void *thread_info) {
    thread_info_t *arg = (thread_info_t *) thread_info;

    const char* end = arg->buf + arg->end;
    const char* start = arg->buf + arg->start;
    char *c = (char*) start;
    char *record = (char*) start;

    ___ASSERT(('A' <= c[0] && c[0] <= 'Z') || c[0] == "İ"[0] || c[0] == "Ü"[0], "token-mmap\n");

    for (; c < start + 32; c++)
        if (*c == '\n')
            record = process_record_mmap_slowstart(record, c, &arg->stations);

    for (; c < end - 32; c += 32) {
        for (int mask = get_newline_positions_mask_in_chunk(c); mask; mask &= mask - 1) {
            const int newline_pos = __builtin_ctz(mask);
            record = process_record(record, c + newline_pos, &arg->stations);
        }
    }

    // printf("3\n");
    for (; c < end; c ++)
        if (*c == '\n')
            record = process_record(record, c, &arg->stations);

    return NULL;
}

#endif //PROCESS_MMAP_H
