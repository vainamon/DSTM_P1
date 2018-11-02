
#include <stdio.h>
//#include <pthread.h>
#include <stdint.h>
//extern "C"{
#include "/home/igor/test/dstm_pthread.h"
//}
#include <malloc.h>

//void exit(int n)
//{
//}

typedef struct thread_data {
  int id;
  int* val;
} thread_data_t;

void* blah_blah_foo(void* data)
{
  //thread_data_t *d = (thread_data_t *)data;
  //d->val[d->id]=120+8*d->id;
  int* gtid;
  int a;
  __tm_atomic{
    gtid = (int*)data;
    a = *gtid;
  }
  printf("Inside thread gtid<%d>\n",a);
  return 0;
}
 
int main()
{
  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  
  int nb_threads = 1;

  int set[2];
  
  int i;

  if ((data = (thread_data_t *)malloc(nb_threads * sizeof(thread_data_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  if ((threads = (pthread_t *)malloc(nb_threads * sizeof(pthread_t))) == NULL) {
    perror("malloc");
    exit(1);
  }

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
 
  char test_str[] = "blah_blah_foo"; 
  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread #%d 0x%llx\n",i,&blah_blah_foo);
    data[i].val = set;
    data[i].id = i;
    if (pthread_create(&threads[i], &attr, test_str, (void *)(&data[i])) != 0) {
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
  
  for (i = 0; i < nb_threads; i++) {
    printf("%d\n",set[i]);
  }

  fflush(stdout);
  free(threads);
  free(data);

  return 0;
}
