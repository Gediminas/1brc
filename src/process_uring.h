#ifndef PROCESS_URING_H
#define PROCESS_URING_H

#include "defs.h"
#include "process_common.h"
#include "uring_file_reader.h"
#include <sys/mman.h> //mlock

static void *process_fsegment_uring(void *thread_info) {
    thread_info_t *arg = (thread_info_t *) thread_info;

    mlock(&arg->stations, sizeof(arg->stations)); //PERF: Maybe improves a bit, maybe not

    char *buf = NULL;
    char* record = NULL;
    uint16_t buf_len = 0;
    uint16_t pos = 0;

    ring_file_reader_t reader = rfr_create(arg->fd, arg->start, arg->end);

    record = reader.buf_start + reader.buf_len;
    *(record-1) = '\n'; // Required because in case of 3-letter city-name hashing takes more 1 byte before beginning (which is always '\n') 

    if (LIKELY(!arg->cfg.skip_align)) {
        // Slow processing till aligned byte [tag: SLOWSTART]
        // PERF: Seems to speedup some 0-5% (and somewhat more stable time)

        const uintptr_t aligned_address = (arg->start + DISK_CLUSTER - 1) & ~(DISK_CLUSTER - 1);
        if (arg->start != aligned_address) {
            const ssize_t start_offset = aligned_address - arg->start;

            const int fd = open(arg->fname, O_RDONLY | O_NONBLOCK);
            ___EXPECT(fd, "file open thread");

            record -= start_offset;
            buf = record;
            *(record-1) = '\n'; // Required because in case of 3-letter city-name hashing takes more 1 byte before beginning (which is always '\n') 

            const int ret = lseek(fd, arg->start, SEEK_SET);
            ___EXPECT(ret != -1, "file seek align begin");

            const ssize_t bytes_read = read(fd, buf, start_offset);
            ___EXPECT(bytes_read == start_offset, "slow-start-fread");

            close(fd);

            reader.fpos += start_offset;

            *(buf-1) = '\n';// Required because in case of 3-letter city-name hashing takes more 1 byte before beginning (which is always '\n') 

            for (ssize_t pos = 0; pos < start_offset; pos++) {
                if (*(buf+pos) == '\n')
                    record = process_record(record, buf+pos, &arg->stations);
            }
        }
    }

    // Fast processing using simd on aligned buffer part
    while (rfr_request_next_blocks(&reader)) {
        record -= RING_ENTRIES * READ_BLOCK_LEN; // Previous token is always in the last block, so move it to pre-data block

        while (reader.blocks_in_queue) {
            const int nr = rfr_wait_for_block(&reader);
            buf     = reader.buf[nr];
            buf_len = reader.lengths[nr];

            for (pos = 0; pos <= buf_len - 32; pos += 32) {
                char *chunk = buf + pos;

                for (int mask = get_newline_positions_mask_in_chunk(chunk); mask; mask &= mask - 1) {
                    const int newline_pos = __builtin_ctz(mask);
                    record = process_record(record, chunk + newline_pos, &arg->stations);
                }
            }
        }
    }

    // Slow processing on remainig data (less than simd block - 32-byte/256-bit)
    for (; pos < buf_len; pos++)
        if (buf[pos] == '\n')
            record = process_record(record, (char*) &buf[pos], &arg->stations);

    rfr_destroy(&reader);
    munlock(&arg->stations, sizeof(arg->stations));
    return NULL;
}

#endif //PROCESS_URING_H
