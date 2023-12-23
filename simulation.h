#include "map.h"

typedef struct {
    size_t index;
    size_t ticks;
    map_t *maps;
} sim_t;

sim_t sim_new(size_t width, size_t height, size_t ticks, map_t start);
void sim_free(sim_t s);
void sim_next(sim_t *s);
void sim_draw(sim_t s);