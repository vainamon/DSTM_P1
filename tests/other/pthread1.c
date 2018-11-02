#include <stdio.h>
#include <stdint.h>

#include "/home/igor/test/dstm_pthread.h"
#include "/home/igor/test/dstm_malloc.h"
#include "/home/igor/test/dstm_barrier.h"

#include <malloc.h>
#include <time.h>

//#include <pthread.h>

#define ARRAY_SIZE 100000

typedef struct thread_data {
  int id;
  int *val;
  int size;
  int tnum;
  barrier_t *b;
} thread_data_t;

barrier_t *loc_b = NULL;
gasnett_mutex_t mtx_b = GASNETT_MUTEX_INITIALIZER;

void* blah_blah_foo(void* data)
{
  thread_data_t *d = (thread_data_t *)data;
  //d->val[d->id]=120+8*d->id;
  int gtid;
  int *val = NULL;
  int size;
  long sum = 0;
  int part;
  int tnum;
  int down;
  int up;
  int i = 0;
  barrier_t *barrier;
  int barrier_id;

  __tm_atomic{
    gtid = d->id;
    size = d->size;
    tnum = d->tnum;
    barrier = d->b;
    barrier_id = barrier->id;
  }
/*  
  if (gasnet_mynode() != 0){
    gasnett_mutex_lock(&mtx_b);
    if (loc_b == NULL){
      loc_b = (barrier_t*)malloc(sizeof(barrier_t));
      loc_b->id = barrier_id;
      barrier_init(loc_b,barrier_calculate_threads(tnum));
    }
    gasnett_mutex_unlock(&mtx_b);
    barrier_cross(loc_b);
  }else
    barrier_cross(barrier);
*/
  part = size/tnum;
  down = (gtid-1)*part;
  up = gtid*part-1;
  if(gtid == tnum)
    up = size-1;

  int div = gtid*10;

  __tm_atomic{
    val = d->val;
    for(i = 0; i<ARRAY_SIZE; i++){
	if ((i % div) == 0)
	  val[i] = 2; 
	sum += val[i];    
    }
  }

  printf("Inside thread node<%d>, gtid<%d>, size<%d>, sum<%d>\n",gasnet_mynode(),gtid,size,sum);
  fflush(stdout);
/*
  if (gasnet_mynode() != 0){
    gasnett_mutex_lock(&mtx_b);
    if (loc_b != NULL){
      free(loc_b);
      loc_b = NULL;
    }
    gasnett_mutex_unlock(&mtx_b);
  }
*/
  return 0;
}
 
int main()
{
  struct timeval start, end;

  gettimeofday(&start,NULL);
/*  gasnett_tick_t start, end;
  start = gasnett_ticks_now();*/

  thread_data_t *data;
  dstm_pthread_t *threads;
  dstm_pthread_attr_t attr;
  
  int nb_threads = 16;

  int i;
 
  barrier_t* barrier = (barrier_t*)dstm_malloc(sizeof(barrier_t));

  barrier->id = 10000;

//  barrier_init(barrier,barrier_calculate_threads(nb_threads));

  if ((data = (thread_data_t *)dstm_malloc(nb_threads * sizeof(thread_data_t))) == NULL) {
    perror("dstm_malloc");
    exit(1);
  }

  if ((threads = (dstm_pthread_t *)malloc(nb_threads * sizeof(dstm_pthread_t))) == NULL) {
    perror("malloc");
    exit(1);
  }

  dstm_pthread_attr_init(&attr);
  dstm_pthread_attr_setdetachstate(&attr, DSTM_PTHREAD_CREATE_JOINABLE);

  int* array = (int*)dstm_malloc(sizeof(int)*ARRAY_SIZE);

  int *arr_ptr = (int*)((long)array & 281474976710655);

  memset(arr_ptr,1,ARRAY_SIZE);
  for(i=0;i<ARRAY_SIZE;i++)
	arr_ptr[i]=1;
    
  char test_str[] = "blah_blah_foo"; 
  for (i = 0; i < nb_threads ; i++) {
    thread_data_t *ptr = (thread_data_t*)((long)data & 281474976710655);
    ptr[i].val = array;
    ptr[i].id = i+1;
    ptr[i].size=ARRAY_SIZE;
    ptr[i].tnum=nb_threads;
    ptr[i].b = barrier;
  }
  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread #%d 0x%llx\n",i,&blah_blah_foo);
    if (dstm_pthread_create(&threads[i], &attr, test_str, (void *)(&data[i])) != 0){
      fprintf(stderr, "Error creating thread\n");
      exit(1);
    }
  }

  dstm_pthread_attr_destroy(&attr);
/*
  barrier_cross(barrier);
*/
  printf("STOPPING...\n");

  for (i = 0; i < nb_threads; i++) {
    if (dstm_pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
  }

  free(threads);
  dstm_free(data);
  dstm_free(array);
  dstm_free(barrier);

  gettimeofday(&end,NULL);

  uint64_t time_ns = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
/*  end = gasnett_ticks_now();
  uint64_t time_ns = gasnett_ticks_to_ns(end-start);*/
  printf("TIME: %f\n", (double)(time_ns)/1000);
  return 0;
}
