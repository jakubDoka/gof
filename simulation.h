#include "map.h"

typedef struct {
    size_t index;
    size_t ticks;
    map_t *maps;
} sim_t;

sim_t sim_new(map_t start, size_t ticks);
void sim_free(sim_t s);
void sim_next(sim_t *s);
