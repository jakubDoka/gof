#include "simulation.h"

#define LIST_WORLDS 0
#define SAVE_WORLD 1
#define LOAD_WORLD 2

typedef struct {
  int type;
  union {
    struct {
      int n;
      char **names;
    } list_worlds;
    struct {
      char *name;
      map_t *world;
    } save_world;
    struct {
      map_t *name;
    } load_world;
  } data;
} message_t;
