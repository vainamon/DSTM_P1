#include <stdint.h>

#include "gasnet_common.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct barrier {
  gasnett_cond_t complete;
  gasnett_mutex_t mutex;
  int count;
  volatile int crossing;
  int id;
  struct timeval start;
} barrier_t;

void barrier_init(barrier_t *b, int n);

void barrier_cross(barrier_t *b, int local_wait);

int barrier_calculate_threads(int tnum);

int barrier_canbe_destroyed(barrier_t *b);

int barrier_elapsed(barrier_t *b);

#ifdef __cplusplus
}
#endif
