#include "map.h"
#include <stdlib.h>
#include <string.h>

static inline size_t max(size_t a, size_t b) { return a > b ? a : b; }

size_t alloc_size(size_t width, size_t height) {
    return max(width * height * sizeof(size_t) / MAP_SEGMENT_SIZE, 1);
}

map_t map_new(size_t width, size_t height) {
    size_t *data = malloc(alloc_size(width, height));
    return (map_t) {width, height, data};
}

void map_free(map_t m) { free(m.data); }

void map_set(map_t m, size_t x, size_t y, bool value) {
    size_t index = MAP_PROJECT(x, y, m.width);
    size_t *segment = &m.data[MAP_SEGMENT_INDEX(index)];
    size_t mask = 1ULL << MAP_SEGMENT_OFFSET(index);
    mask = value ? mask : ~mask;
    *segment = value ? *segment | mask : *segment & mask;
}

bool map_get(map_t m, size_t x, size_t y) {
    size_t index = MAP_PROJECT(x, y, m.width);
    size_t segment = m.data[MAP_SEGMENT_INDEX(index)];
    size_t mask = 1ULL << MAP_SEGMENT_OFFSET(index);
    return (segment & mask) != 0;
}

void map_clear(map_t m) { memset(m.data, 0, alloc_size(m.width, m.height)); }
