#include "simulation.h"
#include <stdlib.h>
#include <time.h>

int main(int n, char **args) {
    srand(time(NULL));
    size_t width = 10;
    size_t height = 10;
    size_t ticks = 10;

    if (n >= 3) {
        width = atoi(args[1]);
        height = atoi(args[2]);
    }

    if (n >= 4) {
        ticks = atoi(args[3]);
    }

    map_t m = map_new(width, height);

    for (size_t i = 0; i < m.width; i++) {
        for (size_t j = 0; j < m.height; j++) {
            double rnd = (double) rand() / RAND_MAX;
            map_set(m, i, j, rnd <= 0.2);
        }
    }

    sim_t s = sim_new(width, height, ticks, m);
    sim_draw(s);

    for (size_t i = 0; i < ticks; i++) {
        sim_next(&s);
        sim_draw(s);
    }

    sim_free(s);

    return 0;
}
