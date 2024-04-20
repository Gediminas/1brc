#include "defs.h"
#include "station.h"
#include "process_mmap.h"
#include "process_uring.h"

#include <pthread.h>

static config_t parse_args(int argc, char *argv[]) {
    if (argc < 2 || argv[1][0] == '-')
        goto usage;

    config_t cfg;
    memset(&cfg, 0, sizeof(config_t));

    cfg.fname = argv[1];
    cfg.cpus  = sysconf(_SC_NPROCESSORS_ONLN);

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--cpu") == 0) {
            if (++i >= argc)
                goto usage;
            cfg.cpus = atoi(argv[i]);
            continue;
        }

        if (strcmp(argv[i], "--mmap") == 0) {
            cfg.mmap = true;
            continue;
        }

        if (strcmp(argv[i], "--skip-align") == 0) {
            cfg.skip_align = true;
            continue;
        }

        if (strcmp(argv[i], "--debug") == 0) {
            cfg.debug = true;
            continue;
        }

        goto usage;
    }

    ___EXPECT(strlen(cfg.fname) > 0, "arg-fname");
    ___EXPECT(cfg.cpus > 0, "arg-cpus");
    return cfg;

usage:
    printf("\nUsage: %s <path-to-measurements-file> [--cpu N  --mmap  --debug]\n", argv[0]);
    printf("  where:\n");
    printf("  --cpu N      - Number of threads/cpus to use, if not set then get from the system\n");
    printf("  --mmap       - Use `mmap` for file reading instead of default `io_uring`\n");
    printf("  --debug      - Output for debugging\n");
    printf("  --skip-align - Skips initial (4096-byte) disk cluster alignment before reads (io_uring only)\n");
    printf("\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    const config_t cfg = parse_args(argc, argv);

    const int fd = open(cfg.fname, O_RDONLY | O_NONBLOCK);
    ___EXPECT(fd, "file open");

    const size_t fsize = get_file_size(fd);
    ___EXPECT(fsize >= (size_t) cfg.cpus * MIN_RECORD_WITH_SEP_BYTES, "file-size-min-record");
    ___EXPECT(cfg.mmap || fsize >= (size_t) cfg.cpus * DISK_CLUSTER, "file-size-cluster");

    mlock(hash_to_value_simd, sizeof(hash_to_value_simd)); //PERF: Maybe improves a bit, maybe not
    // mlock(swapped_hash_to_value_simd, sizeof(swapped_hash_to_value_simd)); // [tag: HASH-SWAP]

    char* mmap_buf = cfg.mmap ? init_mmap(fd, fsize) : NULL;

    thread_info_t th_args[cfg.cpus];
    memset(&th_args, 0, sizeof(thread_info_t) * cfg.cpus);
    const size_t seg_size = (fsize / cfg.cpus) + 1; //+1 as int division can truncate
    size_t pos = 0, pos_end;

    for (int i = 0; i < cfg.cpus; i++) {
        pos_end =  cfg.mmap ? move_buff_split_after_newline(pos + seg_size, fsize, mmap_buf)
                            : move_file_split_after_newline(pos + seg_size, fsize, fd, MAX_RECORD_BYTES);
        ___EXPECT(pos < pos_end, "file too small for splitting")

        th_args[i].start = pos;
        th_args[i].end   = pos_end;
        th_args[i].buf   = mmap_buf;
        th_args[i].fd    = fd;
        th_args[i].cfg   = cfg;
        strcpy(th_args[i].fname, cfg.fname);

        pos = pos_end;
    }

    for (int i = 0; i < cfg.cpus; i++) {
        if (cfg.mmap) {
            const int ret = pthread_create(&th_args[i].tid, NULL, process_fsegment_mmap, &th_args[i]);
            ___EXPECT(ret == 0, "thread create");
        }
        else {
            const int ret = pthread_create(&th_args[i].tid, NULL, process_fsegment_uring, &th_args[i]);
            ___EXPECT(ret == 0, "thread create");
        }
    }

    stations_t merged_stations;
    memset(&merged_stations, 0, sizeof(stations_t));

    for (int i = 0; i < cfg.cpus; i++) {
        thread_info_t *arg = &th_args[i];

        const int ret = pthread_join(arg->tid, NULL);
        ___EXPECT(ret == 0, "thread join");

        merge_stations(&merged_stations, &arg->stations);
    }

    print_sorted_stations_as_json(&merged_stations, cfg.debug);

    munlock(hash_to_value_simd, sizeof(hash_to_value_simd));
    // munlock(swapped_hash_to_value_simd, sizeof(swapped_hash_to_value_simd)); // [tag: HASH-SWAP]

    if (cfg.debug) {
        printf("\n=====================================\n");
        printf("File size: %7.1f MB / %11ld B\n", fsize/1000000.0, fsize);
        printf("Seg. size: %7.1f MB / %11ld B\n", seg_size/1000000.0, seg_size);
        printf("CPUs:      %d\n",                 cfg.cpus);
        printf("MODE:      %s%s\n",               cfg.mmap ? "mmap" : "io_uring", 
            #ifdef SAFE
               " (SAFE)"
            #else
               ""
            #endif
           );
        printf("cities:    %d\n",                 merged_stations.count);
        printf("=======================================\n");
        printf("SIZES:                         .  .\n");
        printf("> stations_t:                 %7lu B\n", sizeof(stations_t));
        printf("> station_data_t->hash2index: %7lu B\n", sizeof(uint16_t[STATION_HASH_MOD]));
        printf("> station_data_t->names:      %7lu B\n", sizeof(char[MAX_STATIONS][MAX_STATION_NAME]));
        printf("> station_data_t->data:       %7lu B\n", sizeof(station_data_t[MAX_STATIONS]));
        printf("> hash_to_value_simd:         %7lu B\n", sizeof(hash_to_value_simd));
        printf("> swapped_hash_to_value_simd: %7lu B\n", sizeof(swapped_hash_to_value_simd));
        printf("=======================================\n");
    }

    if (cfg.mmap) {
        munmap(mmap_buf, fsize);
    }

    close(fd);
    return EXIT_SUCCESS;
}
