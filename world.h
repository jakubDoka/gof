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
} map;

map map_new(size_t width, size_t height);
void map_free(map m);
void map_set(map m, size_t x, size_t y, bool value);
bool map_get(map m, size_t x, size_t y);
void map_clear(map m);
void map_tick(map m);
