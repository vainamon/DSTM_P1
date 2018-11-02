#include <stdio.h>
#include <stdint.h>
//#include "/home/igor/test/dstm_pthread.h"
//#include "/home/igor/test/dstm_malloc.h"
#include <malloc.h>
#include <pthread.h>
#include <time.h>

#define ARRAY_SIZE 100000

typedef struct thread_data {
  int id;
  int* val;
  int size;
  int tnum;
  unsigned int seed;
} thread_data_t;

pthread_mutex_t GLMTX = PTHREAD_MUTEX_INITIALIZER;

void* blah_blah_foo(void* data)
{
  thread_data_t *d = (thread_data_t *)data;
  //d->val[d->id]=120+8*d->id;
  int gtid;
  int *val;
  int size;
  int sum = 0;
  int part;
  int tnum;
  int down, up;
  int i;
  int div;
  
  unsigned int sleep;
//  __tm_atomic{
  pthread_mutex_lock(&GLMTX);
    gtid = d->id;
    val = d->val;
    size = d->size;
    tnum = d->tnum;
    sleep = rand_r(&d->seed) % 500 + 500;
  pthread_mutex_unlock(&GLMTX);

    part = size/tnum;
    down = (gtid-1)*part;
    up = gtid*part-1;
    div = gtid*10;

    if(gtid == d->tnum)
	up = size;

   usleep(sleep);
   pthread_mutex_lock(&GLMTX);
    for(i = 0;i<ARRAY_SIZE;i++){
	if((i % div) == 0)
	  val[i] = 2;
	sum += val[i];    
    }
//  }
  pthread_mutex_unlock(&GLMTX);
  printf("Inside thread down<%d>, up<%d>, gtid<%d>, size<%d>, sum<%d>\n",down,up,gtid,size,sum);
  return 0;
}
 
int main()
{
  struct timespec ts1;
  struct timespec ts2;
  clock_gettime(CLOCK_REALTIME,&ts1);

  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  
  int nb_threads = 8;

  int i;

  if ((data = (thread_data_t *)malloc(nb_threads * sizeof(thread_data_t))) == NULL) {
    perror("dstm_malloc");
    exit(1);
  }

  if ((threads = (pthread_t *)malloc(nb_threads * sizeof(pthread_t))) == NULL) {
    perror("malloc");
    exit(1);
  }

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  int* array = malloc(sizeof(int)*ARRAY_SIZE);

  for(i=0;i<ARRAY_SIZE;i++)
	array[i]=1;
    
  char test_str[] = "blah_blah_foo"; 
  srand((int)time(0));
  for (i = 0; i < nb_threads; i++) {
    data[i].val = array;
    data[i].id = i+1;
    data[i].size=ARRAY_SIZE;
    data[i].tnum=nb_threads;
    data[i].seed=rand();

    printf("Creating thread #%d 0x%llx\n",i,&blah_blah_foo);
    if (pthread_create(&threads[i], &attr, blah_blah_foo, (void *)(&data[i])) != 0){
      fprintf(stderr, "Error creating thread\n");
      exit(1);
    }
  }
  pthread_attr_destroy(&attr);
  
  printf("STOPPING...\n");

  for (i = 0; i < nb_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
  }

  free(threads);
  free(data);
  free(array);
  
  clock_gettime(CLOCK_REALTIME,&ts2); 

  uint64_t time_ns = ts2.tv_nsec-ts1.tv_nsec;
  printf("TIME: %f\n", (double)(time_ns)/1000000000);

  pthread_mutex_destroy(&GLMTX);
  return 0;
}
