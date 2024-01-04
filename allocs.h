
#include <stdlib.h>

static int gof_rc = 0;

void *gof_alloc(size_t);
void gof_free(void *);
void gof_exit_and_check(void);
