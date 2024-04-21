#ifndef STATION_H
#define STATION_H

#include "defs.h"
#include "extraction.h"
#include "utils.h"
#include <stdbool.h>
#include <string.h>

typedef struct {
    int16_t  min; // PERF:*_fast*_t types work 10-20% worse, 
    int16_t  max; //      likely because they are larger and increase cache misses
    uint32_t cnt;

#ifndef SAFE
    int32_t  sum; // PERF: Faster; For real-world temperatures and using threads should be enough
#else
    int64_t  sum; // PERF: Slower, likely due to cache misses; This is totaly safe, should work even with data from Antarctida or Venus
#endif

    uint16_t hash;
} STRUCT_ALIGN(16) station_data_t;


typedef struct {
    uint16_t       count;
    station_data_t data[MAX_STATIONS];
    char           names[MAX_STATIONS][MAX_STATION_NAME];
    uint16_t       hash2index[STATION_HASH_MOD];
} STRUCT_ALIGN(16) stations_t;

typedef struct {
    uint16_t station_index;
    char    *station_name_ref; // not owned
} STRUCT_ALIGN(16) station_sort_entry_t;


INLINE station_data_t* get_station(stations_t *stations, char* name, size_t name_len) {
    const uint16_t hash = extract_city_hash(name, name_len);

    uint16_t mhash = hash;                    //#1
    // uint16_t mhash = hash % STATION_HASH_MOD; //#2

    station_data_t *s;
    size_t index;

    #ifdef SAFE
        #ifdef DEBUG
            size_t stat_collisions = 0;
        #endif
        loop_next_hash:
    #endif

    index = stations->hash2index[mhash]; // PERF: 130KB map; Cache misses?

    // PERF: Branch prediction (1 bln LIKELY vs 413 stations x cpus)
    if (LIKELY(index != 0)) {
        s = &stations->data[index];

        #ifdef SAFE
          // PERF: some slowdown with this, but still not safe
          // if (UNLIKELY(s->hash != hash)) {

          // PERF: safe but 2x slowdown with this !!!
          if (UNLIKELY(memcmp(name, stations->names[index], name_len) != 0)) {

            #ifdef DEBUG
                stat_collisions++;
            #endif

            mhash++;
            mhash %= STATION_HASH_MOD;
            goto loop_next_hash;
          }
        #endif
    }
    else {
        stations->hash2index[mhash] = index = ++stations->count;
        s = &stations->data[index];
        s->hash = hash;
        memcpy(&stations->names[index], name, name_len);
        stations->names[index][name_len] = 0;

        #if defined(SAFE) && defined(DEBUG)
          if (stat_collisions) {
            printf("collision: %3u  0x%x  %s  collisions=%ld\n", stations->count,
                   s->hash, stations->names[index], stat_collisions);
            exit(EXIT_FAILURE);
          }
        #endif
    }

    return s;
}

static inline void merge_station(stations_t *merged_stations, station_data_t *other_station, stations_t *other_stations, uint16_t index2) {
    const uint32_t hash = other_station->hash;
    uint32_t mhash = hash % STATION_HASH_MOD;
    station_data_t *s;
    size_t index;

    loop:
    index = merged_stations->hash2index[mhash];

    if (LIKELY(index != 0)) {
        s = &merged_stations->data[index];

        #ifdef SAFE
            if (UNLIKELY(strcmp(merged_stations->names[index], other_stations->names[index2]) != 0)) {
        #else
            if (UNLIKELY(s->hash != hash)) {
        #endif
                mhash++;
                mhash %= STATION_HASH_MOD;
                goto loop;
            }
    }
    else {
        merged_stations->hash2index[mhash] = index = ++merged_stations->count;
        s = &merged_stations->data[index];
        s->hash = hash;
        memcpy(merged_stations->names[index], other_stations->names[index2], MAX_STATION_NAME);
    }

    s->min = min16(s->min, other_station->min);
    s->max = max16(s->max, other_station->max);
    s->sum += other_station->sum;
    s->cnt += other_station->cnt;
}

static inline void merge_stations(stations_t *merged_stations, stations_t *other_stations) {
    //0 station is not used
    for (uint16_t i = 1; i <= other_stations->count; i++)
        merge_station(merged_stations, &(other_stations->data[i]), other_stations, i);
}


static inline int compare_station_name(const void *a, const void *b) {
    const station_sort_entry_t *ea = (const station_sort_entry_t *) a;
    const station_sort_entry_t *eb = (const station_sort_entry_t *) b;
    return strcmp(ea->station_name_ref, eb->station_name_ref);
}

static inline void print_sorted_stations_as_json(stations_t *stations, bool debg) {
    station_sort_entry_t entries[MAX_STATIONS];

    for (uint16_t i = 1; i < stations->count+1; i++) {
        station_sort_entry_t *h = &entries[i];
        h->station_name_ref = stations->names[i];
        h->station_index = i;
    }

    qsort(entries+1, stations->count, sizeof(station_sort_entry_t), compare_station_name);

    printf("{");
    for (uint16_t i = 1; i < stations->count+1; i++) {
        if (debg && i > 20 && i < stations->count - 20) {
            printf(".");
            continue;
        }
        const station_sort_entry_t *entry = &entries[i];
        const station_data_t *data = &stations->data[entry->station_index];
        const double min = 0.1 * data->min;
        const double max = 0.1 * data->max;
        const double avg = 0.1 * data->sum / data->cnt;
        printf("%s=%.1f/%.1f/%.1f", entry->station_name_ref, min, avg, max);
        if (LIKELY(i < stations->count))
            printf(", ");
    }
    printf("}\n");
}

#endif //STATION_H
