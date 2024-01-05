#include "simulation.h"
#include <stdlib.h>

sim_t sim_new(map_t start, size_t ticks) {
    map_t *maps = malloc(sizeof(map_t) * (ticks + 1));

    for (int i = 0; i <= ticks; ++i) {
        maps[i] = i == 0 ? start : map_new(start.width, start.height);
    }

    return (sim_t){0, ticks, maps};
}

void sim_free(sim_t s) {
    if (s.maps == NULL) {
        return;
    }

    for (int i = 0; i <= s.ticks; ++i) {
        map_free(s.maps[i]);
    }

    free(s.maps);
    s.maps = NULL;
}

void sim_next(sim_t *s) {
    map_t m_prev = s->maps[s->index];
    map_t m_next = s->maps[++s->index];

    for (size_t y = 0; y < m_prev.height; y++) {
        size_t start_y = y == 0 ? 0 : y - 1;
        size_t end_y = y == m_prev.height - 1 ? m_prev.height - 1 : y + 1;

        for (size_t x = 0; x < m_prev.width; x++) {
            size_t start_x = x == 0 ? 0 : x - 1;
            size_t end_x = x == m_prev.width - 1 ? m_prev.width - 1 : x + 1;
            size_t count = 0;

            for (size_t i = start_y; i <= end_y; i++) {
                for (size_t j = start_x; j <= end_x; j++) {
                    if (j != x || i != y) {
                        count += map_get(m_prev, j, i);
                    }
                }
            }

            bool alive = map_get(m_prev, x, y);
            map_set(m_next, x, y,
                    alive ? count == 2 || count == 3 : count == 3);
        }
    }
}
