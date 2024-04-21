#ifndef URING_FILE_READER_H
#define URING_FILE_READER_H

#include "defs.h"
#include "utils.h"

#include <string.h>
#include <liburing.h>

typedef struct io_uring io_uring;

typedef struct {
    io_uring     ring;

    // file
    const size_t fend;
    size_t       fpos;

    // buffer
    const size_t buf_len;
    char        *buf_start;
    char        *buf[RING_ENTRIES];

    uint16_t     lengths[RING_ENTRIES];  // bytes read for every buffer (all should be READ_BLOCK_LEN, except the very last)
    size_t       total_blocks_requested; // total blocks requested
    uint16_t     blocks_in_queue;        // blocks requested but not received/processed
    uint16_t     next_expected_block;    // to catch out-of-order blocks
    bool         on_hold[RING_ENTRIES];  // true - blocks received out-of-order, on hold till the expected arives
                                         // happens when file not cached. IOSQE_IO_LINK is slow for cached data

    #ifdef DEBUG
        size_t   stats_out_of_order;
    #endif
} ring_file_reader_t;


static inline ring_file_reader_t rfr_create(int fd, size_t fstart, size_t fend) {
    ring_file_reader_t self = {
        .fend          = fend,
        .fpos          = fstart,
        .buf_len       = RING_BUFFER_WRAPUP_LEN + READ_BLOCK_LEN * RING_ENTRIES,
        .buf_start     = (char*) aligned_alloc(ALIGNMENT, self.buf_len),
        .total_blocks_requested = 0,
        .blocks_in_queue      = 0,
        .next_expected_block    = 0,
        #ifdef DEBUG
            .stats_out_of_order = 0
        #endif
    };

    memset(self.on_hold, 0, RING_ENTRIES * sizeof(bool));

    for (int i = 0; i < RING_ENTRIES; i++) {
        self.buf[i] = self.buf_start + RING_BUFFER_WRAPUP_LEN + READ_BLOCK_LEN * i;
    }

    struct io_uring_params params;
    memset(&params, 0, sizeof(params));
    params.flags |= IORING_SETUP_SUBMIT_ALL;
    params.flags |= IORING_SETUP_SINGLE_ISSUER;
    params.flags |= IORING_SETUP_NO_MMAP;
    // params.flags |= IORING_SETUP_SQ_AFF;

    int ret = io_uring_queue_init_params(RING_ENTRIES + RING_EXTRA_ENTRIES, &self.ring, &params);
    // int ret = io_uring_queue_init(RING_ENTRIES + RING_EXTRA_ENTRIES, &self.ring, 0);
    ___EXPECT(ret == 0, "ring-init");

    const int fds[1] = { fd };
    ret = io_uring_register_files(&self.ring, fds, 1);
    ___EXPECT(ret == 0, "ring-reg-file");

    return self;
}

static inline void rfr_destroy(ring_file_reader_t *self) {
    #ifdef DEBUG
        if (self->stats_out_of_order)
            printf("STAT: blocks out or order: %ld\n", self->stats_out_of_order);
    #endif

    free(self->buf_start);

    const int ret = io_uring_unregister_files(&self->ring);
    ___EXPECT(ret == 0, "ring-reg-file");

    io_uring_queue_exit(&self->ring);
}

static inline bool rfr_request_next_block(ring_file_reader_t *reader) {
    const int64_t remains = reader->fend - reader->fpos;

    if (remains <= 0)
        return false;

    struct io_uring_sqe *sqe = io_uring_get_sqe(&reader->ring);
    ___EXPECT(sqe, "sqe get");

    const size_t nr = reader->total_blocks_requested % RING_ENTRIES;

    char* buf = reader->buf[nr];

    const int64_t to_read = min64(remains, READ_BLOCK_LEN);
    // const size_t to_read = remains < READ_BLOCK_LEN ? remains : READ_BLOCK_LEN;
    
    // io_uring_prep_read(sqe, reader->fd, buf, to_read, reader->fpos);
    io_uring_prep_read(sqe, 0, buf, to_read, reader->fpos);
    io_uring_sqe_set_data(sqe, (void*) (size_t) nr);
    io_uring_sqe_set_flags(sqe, IOSQE_FIXED_FILE);
    // io_uring_sqe_set_flags(sqe, IOSQE_IO_LINK); // PERF: with it - 20-40% slower (sometimes much slower) with a cahced file

    reader->lengths[nr] = to_read;
    reader->total_blocks_requested ++;
    reader->fpos += to_read;
    
    return true;
}

static inline uint8_t rfr_request_next_blocks(ring_file_reader_t *reader) {
    for (reader->blocks_in_queue = 0; reader->blocks_in_queue < RING_ENTRIES && rfr_request_next_block(reader); reader->blocks_in_queue++);

    if (!reader->blocks_in_queue)
        return 0;

    // Imitate ring buffer
    memcpy(reader->buf_start, reader->buf_start + reader->buf_len - RING_BUFFER_WRAPUP_LEN, RING_BUFFER_WRAPUP_LEN);

    const int ret = io_uring_submit(&reader->ring);
    ___EXPECT(ret >= 0, "sqe submit");

    return reader->blocks_in_queue;
}

static inline int rfr_wait_for_block(ring_file_reader_t *reader) {
    size_t nr;

    if (reader->on_hold[reader->next_expected_block]) {
        reader->on_hold[reader->next_expected_block] = 0;
        nr = reader->next_expected_block;
        goto done;
    }

    struct io_uring_cqe *cqe;

    for (;;) {
        const int ret = io_uring_wait_cqe(&reader->ring, &cqe);
        ___EXPECT((ret == 0) && cqe, "wait cqe");

        nr = (size_t) io_uring_cqe_get_data(cqe);
        io_uring_cqe_seen(&reader->ring, cqe);

        if (nr == reader->next_expected_block)
            break;

        reader->on_hold[nr] = true;

        #ifdef DEBUG
            reader->stats_out_of_order ++;
        #endif
    }

done:
    reader->next_expected_block++;
    reader->next_expected_block %= RING_ENTRIES;
    reader->blocks_in_queue--;
    return nr;
}

#endif //URING_FILE_READER_H
