#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline size_t max(size_t a, size_t b) { return a > b ? a : b; }

size_t map_alloc_size(size_t width, size_t height) {
    return width * height * sizeof(size_t) / MAP_SEGMENT_SIZE + sizeof(size_t);
}

map_t map_new(size_t width, size_t height) {
    size_t *data = malloc(map_alloc_size(width, height));
    return (map_t){width, height, data};
}

void map_free(map_t m) {
    if (m.data == NULL) {
        return;
    }

    free(m.data);
    m.data = NULL;
}

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

void map_gen_rnd(map_t m) {
    for (size_t i = 0; i < m.width; i++) {
        for (size_t j = 0; j < m.height; j++) {
            double rnd = (double)rand() / RAND_MAX;
            map_set(m, i, j, rnd <= 0.2);
        }
    }
}

void map_save(map_t m, char *fn) {
    FILE *file = fopen(fn, "w");

    if (file == NULL) {
        perror("Error opening file!");
        return;
    }

    fprintf(file, "%zu %zu\n", m.width, m.height);

    for (size_t i = 0; i < m.height; i++) {
        for (size_t j = 0; j < m.width; j++) {
            fprintf(file, "%d\n", map_get(m, j, i));
        }
    }

    fclose(file);
}

map_t map_load(char *fn) {
    FILE *file = fopen(fn, "r");

    if (file == NULL) {
        perror("Error opening file!");
        return MAP_NULL;
    }

    size_t width;
    size_t height;
    size_t value;

    if (fscanf(file, "%zu %zu\n", &width, &height) != 2) {
        perror("Error reading dimensions from file!");
        return MAP_NULL;
    }

    map_t m = map_new(width, height);

    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            if (fscanf(file, "%zu\n", &value) != 1) {
                perror("Error reading cells from file!");
                map_free(m);
                return MAP_NULL;
            }
            map_set(m, j, i, value == 1);
        }
    }

    fclose(file);
    return m;
}

void map_clear(map_t m) {
    memset(m.data, 0, map_alloc_size(m.width, m.height));
}
