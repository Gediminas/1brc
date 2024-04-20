#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>


INLINE int16_t min16(int16_t a, int16_t b) {
    // return b + ((a - b) & ((a - b) >> 15)); // #1 branchless but slower
    return a < b ? a : b;                      // #2 compiler optimizes without branches, somewhat better
}

INLINE int16_t max16(int16_t a, int16_t b) {
    // return a - ((a - b) & ((a - b) >> 15)); // #1 branchless but slower
    return a < b ? b : a;                      // #2 compiler optimizes without branches, somewhat better
}

INLINE int64_t min64(int64_t a, int64_t b) {
    // return b + ((a - b) & ((a - b) >> 63)); // #1 branchless, similar speed (used less frequently than min16)
    return a < b ? a : b;                      // #2 compiler makes it branshless, similar speed
}


INLINE size_t get_file_size(int fd) {
    struct stat sb;
    const int ret = fstat(fd, &sb);
    ___EXPECT(ret != -1, "file-stat");

    return sb.st_size;
}

INLINE size_t move_file_split_after_newline(size_t split_end_pos, size_t fsize, const int fd, size_t max_record_size) {
    if (split_end_pos >= fsize)
        return fsize;

    const int ret = lseek(fd, split_end_pos - 1, SEEK_SET); //seek to byte before split (we need it to be newline)
    ___EXPECT(ret != -1, "split-rint-fseek");

    char buffer[max_record_size];
    const ssize_t bytes_read = read(fd, buffer, max_record_size);
    ___EXPECT(bytes_read > 0, "split-rint-fread");

    size_t newline_offset = 0;
    for (; buffer[newline_offset] != '\n' && newline_offset < max_record_size; newline_offset++);
    ___EXPECT(buffer[newline_offset] == '\n', "split-rint-fformat");

    return split_end_pos + newline_offset;
}

INLINE size_t move_buff_split_after_newline(size_t split_end_pos, size_t fsize, char *buffer) {
    if (split_end_pos >= fsize)
        return fsize;

    while (buffer[split_end_pos - 1] != '\n') { split_end_pos++; }

    ___EXPECT(buffer[split_end_pos - 1] == '\n', "split-mmap-fformat");
    return split_end_pos;
}

// INLINE inline void set_stack_size(rlim_t kStackSize) {
//     struct rlimit rl;
//     int result = getrlimit(RLIMIT_STACK, &rl);
//     ___EXPECT(result == 0, "stack get")

//     if (rl.rlim_cur >= kStackSize) {
//         return;
//     }

//     rl.rlim_cur = kStackSize;
//     result = setrlimit(RLIMIT_STACK, &rl);
//     ___EXPECT(result == 0, "stack set")
// }
    
// INLINE void debug_print_32byte_block(char* title, char* block) {
//     char b[33];
//     memcpy(b, block, 32);
//     for (int i = 0; i < 32; i++) {
//         if (b[i] == '\n') {
//             b[i] = '|';
//         }
//     }
//     b[32] = 0;
//     printf("\n%s\t[%s]\n\t _123456789_123456789_123456789_1\n", title, b);
// }

#endif //UTILS_H
