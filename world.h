#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAP_SEGMENT_SIZE sizeof(size_t) * 8
#define MAP_PROJECT(x, y, width) (y * width + x)
#define MAP_SEGMENT_INDEX(x) (x / MAP_SEGMENT_SIZE)
#define MAP_SEGMENT_OFFSET(x) (x % MAP_SEGMENT_SIZE)

typedef struct {
  size_t width;
  size_t height;
  size_t *data;
} map_t;

map_t map_new(size_t width, size_t height);
void map_free(map_t m);
void map_set(map_t m, size_t x, size_t y, bool value);
bool map_get(map_t m, size_t x, size_t y);
void map_clear(map_t m);
void map_tick(map_t m);
