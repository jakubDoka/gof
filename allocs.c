#include "allocs.h"
#include <stdio.h>

void *gof_alloc(size_t size) {
  gof_rc++;
  return malloc(size);
}

void gof_free(void *ptr) {
  gof_rc--;
  free(ptr);
}

void gof_exit_and_check() {
  if (gof_rc != 0) {
    fprintf(stderr, "Memory leak detected: %d\n", gof_rc);
    exit(1);
  }
}
