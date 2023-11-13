#include "world.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

inline size_t max(size_t a, size_t b) { return a > b ? a : b; }

size_t alloc_size(size_t width, size_t height) {
  return max(width * height * sizeof(size_t) / MAP_SEGMENT_SIZE, 1);
}

map map_new(size_t width, size_t height) {
  size_t *data = malloc(alloc_size(width, height));
  return (map){width, height, data};
}

void map_free(map m) { free(m.data); }

inline void map_set(map m, size_t x, size_t y, bool value) {
  size_t index = MAP_PROJECT(x, y, m.width);
  size_t *segment = &m.data[MAP_SEGMENT_INDEX(index)];
  size_t mask = 1 << MAP_SEGMENT_OFFSET(index);
  mask = value ? mask : ~mask;
  *segment = *segment & mask;
}

inline bool map_get(map m, size_t x, size_t y) {
  size_t index = MAP_PROJECT(x, y, m.width);
  size_t segment = m.data[MAP_SEGMENT_INDEX(index)];
  size_t mask = 1 << MAP_SEGMENT_OFFSET(index);
  return (segment & mask) != 0;
}

void map_clear(map m) { memset(m.data, 0, alloc_size(m.width, m.height)); }

void map_tick(map m) {
  for (size_t y = 0; y < m.height; y++) {
    size_t start_y = y == 0 ? 0 : y - 1;
    size_t end_y = y == m.height - 1 ? m.height - 1 : y + 1;
    for (size_t x = 0; x < m.width; x++) {
      size_t start_x = x == 0 ? 0 : x - 1;
      size_t end_x = x == m.width - 1 ? m.width - 1 : x + 1;

      size_t count = 0;
      for (size_t i = start_y; i <= end_y; i++) {
        for (size_t j = start_x; j <= end_x; j++) {
          count += map_get(m, j, i);
        }
      }

      bool alive = map_get(m, x, y);
      map_set(m, x, y, alive ? count == 3 || count == 4 : count == 3);
    }
  }
}
