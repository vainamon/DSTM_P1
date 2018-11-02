/* Copyright (C) 2007  Pascal Felber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include "../../project/src/dstm_pthread/dstm_pthread.h"
#include "../../project/src/dstm_malloc/dstm_malloc.h"
#include "../../project/src/dstm_barrier.h"

//#define DELAY 100

#ifdef ENABLE_REPORTING
#include "tanger-stm-stats.h"
#endif

//#define DEBUG

#ifdef DEBUG
#define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif
#define DEFAULT_DURATION  10000
#define DEFAULT_INITIAL  1024
#define DEFAULT_NB_THREADS 144
#define DEFAULT_RANGE  0xFFFF
#define DEFAULT_SEED  0
#define DEFAULT_UPDATE  80
#define DEFAULT_NB_NODES 24

#define STORE(addr, value) (*(addr) = (value))
#define LOAD(addr) (*(addr))

#include <ctype.h>
#include <stdio.h>
#include <string.h>

//#define MIN(x , y)  (((x) < (y)) ? (x) : (y))
/*	#define TEST = 1 */

#ifdef TEST

void dump(char *ptr, unsigned int ct);

int
main() {
  char *msg = "This is a test. Foo. Bar.\n";

  dump((char *)msg,(1+strlen(msg)));
  printf(msg);

  return (0);
}

#endif


/* dump a line (16 bytes) of memory, starting at pointer ptr for len
   bytes */

void
ldump(char *ptr, unsigned int len) {
  if ( len ) {
    int  i;
    char c;
    len=MIN(16,len);
    /*    printf("\nStart is %10x, Count is %5x, End is %10x",ptr,len,ptr+len); */
    printf("\n%10p ",ptr);

    for( i = 0 ; i < len ; i++ )  /* Print the hex dump for len
                                     chars. */
      printf("%3x",(*(ptr+i)&0xff));

    for( i = len ; i < 16 ; i++ ) /* Pad out the dump field to the
                                     ASCII field. */
      printf("   ");

    printf(" ");
    for( i = 0 ; i < len ; i++ ) { /* Now print the ASCII field. */
      c=0x7f&(*(ptr+i));      /* Mask out bit 7 */

      if (!(isprint(c))) {    /* If not printable */
        printf(".");          /* print a dot */
      } else {
        printf("%c",c);  }    /* else display it */
    }
  }
}

/* Print out a header for a hex dump starting at address st. Each
   entry shows the least significant nybble of the address for that
   column. */

void
head(long st) {
  int i;
  printf("\n   addr    ");
  for ( i = st&0xf ; i < (st&0xf)+0x10 ; i++ )
    printf("%3x",(i&0x0f));
  printf(" | 7 bit ascii. |");
}


/* Dump a region of memory, starting at ptr for ct bytes */

void
dump(char *ptr, unsigned int ct) {
  if ( ct ) {
    int i;

    /*  preliminary info for user's benefit. */
    printf("\nStart is %10p, Count is %5x, End is %10p",
           ptr, ct, ptr + ct);
    head((long)ptr);
    for ( i = 0 ; i <= ct ; i = i+16 , ptr = ptr+16 )
      ldump(ptr,(MIN(16,(ct-i))));

    printf("\n\"Enter\" to continue.\n"); /* Give him/her/it a chance
                                             to examine it. */
    getchar ();
  }
}

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

//#define POLL_T

//static int verbose_flag = 0;

volatile int STOP = 0;
volatile int THREADS = 0;
barrier_t *loc_b = NULL;
gasnett_mutex_t mtx_b = GASNETT_MUTEX_INITIALIZER;

/* ################################################################### *
 * INT SET
 * ################################################################### */

typedef struct node {
  intptr_t val;
  struct node *next;
} node_t;

typedef struct intset {
  node_t *head;
} intset_t;

node_t *new_node(intptr_t val, node_t *next, int gasnet_node)
{
  node_t *node;

  node = (node_t *)dstm_malloc(sizeof(node_t),gasnet_node);
  if (node == NULL) {
    perror("malloc");
    return NULL;
  }

/*  if (gasnet_node == -1){
    node_t *n_node = (node_t*)((long)node & 0xffffffffffff);
    n_node->val = val;
    n_node->next = next;
  }else{*/
    __tm_atomic{
      STORE(&node->val, val);
      STORE(&node->next, next);
    }
//  }
  return node;
}

intset_t *set_new()
{
  intset_t *set;
  node_t *min, *max;

  if ((set = (intset_t *)dstm_malloc(sizeof(intset_t),UINT_MAX)) == NULL) {
    perror("dstm_malloc set_new");
    return NULL;
  }
  max = new_node(INT_MAX, NULL, UINT_MAX);
  min = new_node(INT_MIN, max, UINT_MAX);
  set->head = min;

  return set;
}

void set_delete(intset_t *set)
{
  node_t *node, *next;

#ifdef DEBUG
  printf("\n++> set_delete\n");
  IO_FLUSH; 
#endif
//  __tm_atomic{
    node = set->head;
//  }

  while (node != NULL) {
    __tm_atomic{
      next = node->next;
    }
    dstm_free(node);
    node = next;
  }
  dstm_free(set);
}

int set_size(intset_t *set)
{
  int size = 0;
  node_t *node, *next;

#ifdef DEBUG
  printf("\n++> set_size\n");
  IO_FLUSH;
#endif

  /* We have at least 2 elements */
  //__tm_atomic{
    int cycles = 0;
    node = (node_t *)LOAD(&set->head); 
  __tm_atomic{  
    next = (node_t *)LOAD(&node->next);
    while (next != NULL) {
      size++;
      node = next;
      next = (node_t *)LOAD(&node->next);
//      cycles++;
//      if(cycles > 800){
        //printf("Cycled set_size!\n");
        //fflush(stdout);
//      }
    }
  }
  return size;
}

void print_set(intset_t *set)
{
  node_t *node, *next;
  int i = 0;
  node = (node_t *)LOAD(&set->head);
  printf("\n-------------\nSet:\n");
  __tm_atomic{
    next = (node_t *)LOAD(&node->next);
    while (next != NULL) {
      printf("%d: %p - %d\n",++i,node,(int)LOAD(&next->val));
      node = next;
      next = (node_t *)LOAD(&node->next);
    }
  }
  printf("%d: %p - \n----------------\n",++i,node);
  fflush(stdout);
}

int set_add(intset_t *set, intptr_t val, int notx, int gasnet_node)
{
  int result;
  node_t *prev, *next;
  intptr_t v;
  node_t *n = NULL;

#ifdef DEBUG
  printf("\n++> set_add(%d)\n", val);
  IO_FLUSH;
#endif

  if (notx) {
    prev = set->head;
    next = prev->next;
    while (next->val < val) {
      prev = next;
      next = prev->next;
    }
    result = (next->val != val);
    if (result) {
      prev->next = new_node(val, next, UINT_MAX);
    }
  } else {
    n = new_node(val, next, gasnet_node);
    int cycles = 0;
    int flag = 0;
    __tm_atomic {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = (intptr_t)LOAD(&next->val);
//	printf("%d prev %p next %p val %d\n",cycles, prev, next,v);
#ifdef DELAY
/*  struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = DELAY * 1000;
  if (nanosleep(&timeout, NULL) != 0) {
    perror("nanosleep");
    return 1;
  }*/
#endif
        if (v >= val)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
//        cycles++;
//        if (cycles > 800){
//          printf("Cycled add!\n");
//          fflush(NULL);
//        }
        //if (cycles > 898) break;
      }
      result = (v != val);
      if (result) {
//	printf("store prev %p next %p\n", prev, next);
        STORE(&n->val, val);
        STORE(&n->next, next);
        STORE(&prev->next, n);
      }
    }

    if (!result) dstm_free(n);
  }

  return result;
}

int set_remove(intset_t *set, intptr_t val, int notx)
{
  int result;
  node_t *prev, *next;
  intptr_t v;
  node_t *n;

#ifdef DEBUG
  printf("++> set_remove(%d)\n", val);
  IO_FLUSH;
#endif

  if (notx) {
    prev = set->head;
    next = prev->next;
    while (next->val < val) {
      prev = next;
      next = prev->next;
    }
    result = (next->val == val);
    if (result) {
      prev->next = next->next;
      free(next);
    }
  } else {
    int cycles = 0;
    int flag = 0;
    __tm_atomic {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = (intptr_t)LOAD(&next->val);
#ifdef DELAY
  /*struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = DELAY * 1000;
  if (nanosleep(&timeout, NULL) != 0) {
    perror("nanosleep");
    return 1;
  }*/
#endif
        if (v >= val)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
//        cycles++;
//        if (cycles > 800){
//          printf("Cycled remove!\n");
//          fflush(NULL); 
//        }
        //if (cycles > 898) break;
      }
      result = (v == val);
      if (result) {
        n = (node_t *)LOAD(&next->next);
        STORE(&prev->next, n);
        /* Overwrite deleted node */
        STORE(&next->val, 123456789);
        STORE(&next->next, next);
      }
    }
 
    /* Free the memory after committed execution */
    if (result)
      dstm_free(next);
  }

  return result;
}

int set_contains(intset_t *set, intptr_t val, int notx)
{
  int result;
  node_t *prev, *next;
  intptr_t v;

#ifdef DEBUG
  printf("++> set_contains(%d)\n", val);
  IO_FLUSH;
#endif

  if (notx) {
    prev = set->head;
    next = prev->next;
    while (next->val < val) {
      prev = next;
      next = prev->next;
    }
    result = (next->val == val);
  } else {
    int cycles = 0;
    int flag = 0;
    __tm_atomic {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = (intptr_t)LOAD(&next->val);
#ifdef DELAY
  /*struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = DELAY * 1000;
  if (nanosleep(&timeout, NULL) != 0) {
    perror("nanosleep");
    return 1;
  }*/
#endif
        if (v >= val)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
        cycles++;
//        if (cycles > 800){
//	  printf("Cycled contains!\n");
//	  fflush(NULL);	
//	}
        //if (cycles > 898) break;
      }
      result = (v == val);
    }
  }

  return result;
}

/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data {
  int range;
  int update;
  int nb_add;
  int nb_remove;
  int nb_contains;
  int nb_found;
  int diff;
  unsigned int seed;

  int tnum;
  int *stop;

  intset_t *set;
  barrier_t *barrier;
  int barrier_id;
  char padding[64];
} thread_data_t;

#ifdef POLL_T
void *polling(void *data)
{
  while (STOP == 0){
    gasnet_AMPoll();
  }
}
#endif

void *test(void *data)
{
  int val, last = 0;
  thread_data_t *d = (thread_data_t *)data;
  
  unsigned int seed;
  intset_t *set;
//  barrier_t *barrier;
  int update;
  int range;

  int tnum;
  int *pstop;
  int stop = 0;
  int barrier_id = 10000;

  int diff = 0;
  int nb_add = 0;
  int nb_remove = 0;
  int nb_found = 0;
  int nb_contains = 0;

  __tm_atomic{
    seed = d->seed;    
    set = d->set;
//    barrier = d->barrier;
    update = d->update;
    range = d->range;
    tnum = d->tnum;
//    barrier_id = d->barrier_id;
  }

  int SEED = seed;
  /* Wait on barrier */
  if(gasnet_mynode() != 0){
    gasnett_mutex_lock(&mtx_b);
    if (loc_b == NULL){
      loc_b = (barrier_t*)malloc(sizeof(barrier_t));
      loc_b->id = barrier_id;
      barrier_init(loc_b,barrier_calculate_threads(tnum));
    }
    gasnett_mutex_unlock(&mtx_b);
    barrier_cross(loc_b,1);
  }else
    barrier_cross(d->barrier,1);

  last = -1;
  while (stop == 0) {
    if ((rand_r(&SEED) % 100) < update) {
      if (last < 0) {
        val = (rand_r(&SEED) % range) + 1;
        if (set_add(set, val, 0, UINT_MAX)) {
          diff++;
          last = val;
        }
        nb_add++;
      } else {
        if (set_remove(set, last, 0))
          diff--;
        nb_remove++;
        last = -1;
      }
    } else {
      val = (rand_r(&SEED) % range) + 1;
      if (set_contains(set, val, 0))
        nb_found++;
      nb_contains++;
    }
#ifdef DELAY
  struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = DELAY * 1000;
  if (nanosleep(&timeout, NULL) != 0) {
    perror("nanosleep");
  }
#endif
    __tm_atomic{
      pstop = d->stop;
      stop = *pstop;
    }
  }

/*  __tm_atomic{
    d->nb_add = nb_add;
    d->nb_remove = nb_remove;
    d->nb_found = nb_found;
    d->nb_contains = nb_contains;
    d->diff = diff;
  }*/

  if(gasnet_mynode() != 0){
    barrier_cross(loc_b,0);
  }else
    barrier_cross(d->barrier,0);

  if (gasnet_mynode() != 0){
    gasnett_mutex_lock(&mtx_b);
    if (loc_b != NULL){
      if (barrier_canbe_destroyed(loc_b)){
        /*printf("bar_sync %d ms\n",barrier_elapsed(loc_b));
	fflush(stdout);*/
        free(loc_b);
        loc_b = NULL;
      }
    }
    gasnett_mutex_unlock(&mtx_b);
  }

  __tm_atomic{
    d->nb_add = nb_add;    
    d->nb_remove = nb_remove;
    d->nb_found = nb_found;
    d->nb_contains = nb_contains;
    d->diff = diff;
  }

 /* printf("OUT #%d ads/rms/diff %d/%d/%d \n",++THREADS,nb_add,nb_remove,diff);
  fflush(stdout);*/
}

int main(int argc, char **argv)
{
  /*struct option long_options[] = {
    // These options set a flag
    {"silent",                    no_argument,       &verbose_flag, 0},
    {"verbose",                   no_argument,       &verbose_flag, 1},
    {"debug",                     no_argument,       &verbose_flag, 2},
    {"DEBUG",                     no_argument,       &verbose_flag, 3},
    // These options don't set a flag
    {"help",                      no_argument,       0, 'h'},
    {"duration",                  required_argument, 0, 'd'},
    {"initial-size",              required_argument, 0, 'i'},
    {"num-threads",               required_argument, 0, 'n'},
    {"range",                     required_argument, 0, 'r'},
    {"seed",                      required_argument, 0, 's'},
    {"update-rate",               required_argument, 0, 'u'},
    {0, 0, 0, 0}
  };*/

  intset_t *set;
  int i, c, val, size, reads, updates;
  thread_data_t *data;
  dstm_pthread_t *threads;
  dstm_pthread_attr_t attr;
  barrier_t *barrier;
  struct timeval start, end;
  struct timespec timeout;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = DEFAULT_NB_THREADS;
  int range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  int update = DEFAULT_UPDATE;

#ifdef POLL_T
  dstm_pthread_t *poll_thread;
#endif
  int *stop;
  /*while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hd:i:n:r:s:u:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
     case 0:
       // Flag is automatically set
       break;
     case 'h':
       printf("Usage: %s [-d <int>] [-i <int>] [-n <int>] [-r <int>] [-s <int>] [-u <int>]\n", argv[0]);
       exit(0);
     case 'd':
       duration = atoi(optarg);
       break;
     case 'i':
       initial = atoi(optarg);
       break;
     case 'n':
       nb_threads = atoi(optarg);
       break;
     case 'r':
       range = atoi(optarg);
       break;
     case 's':
       seed = atoi(optarg);
       break;
     case 'u':
       update = atoi(optarg);
       break;
     case '?':
       printf("Use -h or --help for help\n");
       exit(0);
     default:
       exit(1);
    }
  }*/

  assert(duration >= 0);
  assert(initial >= 0);
  assert(nb_threads > 0);
  assert(range > 0);
  assert(update >= 0 && update <= 100);

  printf("Duration     : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Nb threads   : %d\n", nb_threads);
  printf("Value range  : %d\n", range);
  printf("Seed         : %d\n", seed);
  printf("Update rate  : %d\n", update);
  printf("int/long/ptr size: %u/%u/%u\n", (unsigned)sizeof(int), (unsigned)sizeof(long), (unsigned)sizeof(void*));

  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;

  if ((data = (thread_data_t *)dstm_malloc(nb_threads * sizeof(thread_data_t),UINT_MAX)) == NULL) {
    perror("malloc");
    return 1;
  }

  if ((stop = (int *)dstm_malloc(sizeof(int*),UINT_MAX)) == NULL) {
    perror("malloc");
    return 1;
  }

  if ((threads = (dstm_pthread_t *)malloc(nb_threads * sizeof(dstm_pthread_t))) == NULL) {
    perror("malloc");
    return 1;
  }
#ifdef POLL_T
  if ((poll_thread = (dstm_pthread_t *)malloc(sizeof(dstm_pthread_t))) == NULL) {
    perror("malloc");
    return 1;
  }
#endif
  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);
  set = set_new();

  *stop = 0;

  /* Populate set */
  printf("Adding %d entries to set\n", initial);

#ifdef POLL_T
  if (dstm_pthread_create(poll_thread,&attr, "polling", NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  usleep(1000);
#endif
 
/*  for (i = 0; i < initial; i++) {
    val = (rand() % range) + 1;
    set_add(set, val,1,-1);
  }*/

  int nodes = DEFAULT_NB_NODES;
  int up, down, pernode;
  int j = 0;
  up = 0; down = 0; pernode = initial/nodes;

  if (pernode == 0) {
    pernode = 1;
    nodes = initial;
  }

//      dump((char*)set, 160);
  for (j = 0; j<nodes;j++){
    if(j == nodes - 1)
      pernode = initial - (nodes -1)*pernode;
    up += pernode;
    for (i = down; i < up; i++) {
      val = (rand() % range) + 1;
      set_add(set, val,0,j);
      //print_set(set);
//      dump((char*)set, 160);
    }
    down = up;
  }

  size = set_size(set);
  printf("Set size  : %d\n", size);
  
  //print_set(set);

  /* Access set from all threads */
  /*barrier_init(&barrier, nb_threads + 1);*/
  barrier = (barrier_t*)malloc(sizeof(barrier_t));

  barrier->id = 10000;

  barrier_init(barrier,barrier_calculate_threads(nb_threads));


  dstm_pthread_attr_init(&attr);
  dstm_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  printf("<<<------------->>>\n");
  fflush(stdout);
  for (i = 0; i < nb_threads; i++) {
    data[i].range = range;
    data[i].update = update;
    data[i].nb_add = 0;
    data[i].nb_remove = 0;
    data[i].nb_contains = 0;
    data[i].nb_found = 0;
    data[i].diff = 0;
    data[i].seed = rand();
    data[i].set = set;
    data[i].barrier = barrier;
    data[i].tnum = nb_threads;
    data[i].stop = stop;
    data[i].barrier_id = 10000;
  }
  
  for (i = 0; i < nb_threads; i++) {
//    printf("Creating thread %d\n", i);
//    fflush(stdout);
    if (dstm_pthread_create(&threads[i], &attr, "test", (void *)(&data[i])) != 0) {
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }
  }
  
  dstm_pthread_attr_destroy(&attr);

  /* Start threads */
  barrier_cross(barrier,1);

  printf("STARTING...\n");
  fflush(stdout);

  gettimeofday(&start, NULL);
  if (nanosleep(&timeout, NULL) != 0) {
    perror("nanosleep");
    return 1;
  }
  __tm_atomic{
    *stop = 1;
  }

  printf("STOPPING...\n");
  fflush(stdout);

  barrier_cross(barrier,1);

  gettimeofday(&end, NULL);

  /* Wait for threads completion */
  for (i = 0; i < nb_threads; i++) {
    if (dstm_pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      return 1;
    }
  }

#ifdef POLL_T
  STOP = 1;
  if (dstm_pthread_join(*poll_thread, NULL) != 0) {
    fprintf(stderr, "Error waiting for thread completion\n");
    return 1;
  }
#endif

  duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
  reads = 0;
  updates = 0;
  int adds = 0;
  int removes = 0;
  for (i = 0; i < nb_threads; i++) {
    /*printf("Thread %d\n", i);
    printf("  #add      : %d\n", data[i].nb_add);
    printf("  #remove   : %d\n", data[i].nb_remove);
    printf("  #contains : %d\n", data[i].nb_contains);
    printf("  #found    : %d\n", data[i].nb_found);*/
    adds += data[i].nb_add;
    removes += data[i].nb_remove;
    updates += (data[i].nb_add + data[i].nb_remove);
    reads += data[i].nb_contains;
    size += data[i].diff;
  }
  int exp_size = set_size(set);

  printf("Set size    : %d (expected: %d)\n", exp_size, size); 
  printf("Ads/rms     : (%d / %d)\n", adds, removes);
  printf("Duration    : %d (ms)\n", duration);
  printf("#txs        : %d (%f / s)\n", reads + updates, (reads + updates) * 1000.0 / duration);
  printf("#read txs   : %d (%f / s)\n", reads, reads * 1000.0 / duration);
  printf("#update txs : %d (%f / s)\n", updates, updates * 1000.0 / duration);
  fflush(stdout);	
  //print_set(set);
/*  sleep(3);
  int aadds = 0;
  int rremoves = 0;
  for (i = 0; i < nb_threads; i++) {
    aadds += data[i].nb_add;
    rremoves += data[i].nb_remove;
  }
  printf("Ads/rms     : (%d / %d)\n", aadds, rremoves);*/

#ifdef ENABLE_REPORTING
  void* rh = tanger_stm_report_start("app");
  tanger_stm_report_append_string(rh, "name", "linkedlist");
  tanger_stm_report_append_int(rh, "duration_ms", duration);
  tanger_stm_report_append_int(rh, "threads", nb_threads);
  tanger_stm_report_append_int(rh, "size", initial);
  tanger_stm_report_append_int(rh, "updaterate", update);
  tanger_stm_report_append_int(rh, "range", range);
  tanger_stm_report_finish(rh, "app");
  rh = tanger_stm_report_start("throughput");
  tanger_stm_report_append_int(rh, "commits", reads + updates);
  tanger_stm_report_append_double(rh, "commits_per_s", ((double)reads + updates) *1000.0 / duration);
  tanger_stm_report_finish(rh, "throughput");
#endif

  /* Delete set */
  set_delete(set);

  free(threads);
#ifdef POLL_T
  free(poll_thread);
#endif
  dstm_free(data);

  return 0;
}
