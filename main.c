#include "world.h"
#include <stdlib.h>

int main(int n, char **args) {
  size_t width = 1000;
  size_t height = 1000;
  size_t ticks = 1000;

  if (n >= 3) {
    width = atoi(args[1]);
    height = atoi(args[2]);
  }

  if (n >= 4) {
    ticks = atoi(args[3]);
  }

  // Here is Johny
  map m = map_new(width, height);

  for (size_t i = 0; i < ticks; i++) {
    map_tick(m);
  }

  map_free(m);

  return 0;
}
