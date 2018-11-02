/*
 ============================================================================
 Name        : intset_upc.upc
 Author      : Danilov Igor
 Version     :
 Copyright   : Your copyright notice
 Description : UPC intset program
 ============================================================================
 */
#include <upc.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

#include "rbtree.h"

#ifdef DEBUG
#define IO_FLUSH                       fflush(NULL)
#endif

//************************************//

typedef rbtree_t intset_t;

upc_lock_t* list_lock;

shared intset_t* shared shared_rbtree;

//************************************//

shared intset_t * set_new() {
	return rbtree_alloc();
}

void set_delete(shared intset_t *set) {
	rbtree_free(set);
}

int set_size(shared intset_t *set) {
  if (!rbtree_verify(set, 0)) {
    printf("Validation failed!\n");
    exit(1);
  }

  return rbtree_getsize(set);
}

int set_add(shared intset_t *set, intptr_t val, int notx, int gasnet_node) {
int res;
//printf("set add\n");
//fflush(stdout);
  if (notx) {
    res = !rbtree_insert(set, val, val);
  } else {
    upc_lock(list_lock);
      res = !rbtree_insert(set, val, val);
    upc_unlock(list_lock);
  }
//printf("set add out\n");
//fflush(stdout);

  return res;
}

int set_remove(shared intset_t *set, intptr_t val, int notx) {
  int res;
//printf("set remove\n");
//fflush(stdout);

  if (notx) {
    res = rbtree_delete(set, val);
  } else {
    upc_lock(list_lock);
      res = rbtree_delete(set, val);
    upc_unlock(list_lock);
  }
//printf("set remove out\n");
//fflush(stdout);

  return res;
}

int set_contains(shared intset_t *set, intptr_t val, int notx) {
  int res;
//printf("set contains\n");
//fflush(stdout);

  if (notx) {
    res = rbtree_contains(set, val);
  } else {
    upc_lock(list_lock);
      res = rbtree_contains(set, val);
    upc_unlock(list_lock);
  }
//printf("set containts out\n");
//fflush(stdout);

  return res;
}

typedef struct thread_data {
	int nb_add;
	int nb_remove;
	int nb_contains;
	int nb_found;
	int diff;
} thread_data_t;

shared thread_data_t thread_stat[THREADS];

shared int STOP = 0;

shared int seed = 0;
shared int duration = 10000;
shared int initial = 256;
shared int range = 0xFFFF;
shared int update = 20;

void test() {
	int val, last = 0;

	int diff = 0;
	int nb_add = 0;
	int nb_remove = 0;
	int nb_found = 0;
	int nb_contains = 0;

	int SEED = rand();

	last = -1;

	upc_barrier;

	while (STOP == 0) {
		if ((rand_r(&SEED) % 100) < update) {
			if (last < 0) {
				val = (rand_r(&SEED) % range) + 1;
				if (set_add(shared_rbtree, val, 0, -1)) {
					diff++;
					last = val;
				}
				nb_add++;
			} else {
				if (set_remove(shared_rbtree, last, 0))
					diff--;
				nb_remove++;
				last = -1;
			}
		} else {
			val = (rand_r(&SEED) % range) + 1;
			if (set_contains(shared_rbtree, val, 0))
				nb_found++;
			nb_contains++;
		}
	}
        /*printf("OUT #%d ads/rms/diff %d/%d/%d \n",MYTHREAD,nb_add,nb_remove,diff);
         fflush(stdout);*/

	upc_barrier;

	thread_stat[MYTHREAD].nb_add = nb_add;
	thread_stat[MYTHREAD].nb_remove = nb_remove;
	thread_stat[MYTHREAD].nb_found = nb_found;
	thread_stat[MYTHREAD].nb_contains = nb_contains;
	thread_stat[MYTHREAD].diff = diff;
	
/*	printf("OUT #%d ads/rms/diff %d/%d/%d \n",MYTHREAD,nb_add,nb_remove,diff);
	 fflush(stdout);*/
}

int main(int argc, char *argv[])
{
	struct option long_options[] = {
			// These options don't set a flag
			{"help",                      no_argument,       0, 'h'},
			{"duration",                  required_argument, 0, 'd'},
			{"initial-size",              required_argument, 0, 'i'},
			{"range",                     required_argument, 0, 'r'},
			{"seed",                      required_argument, 0, 's'},
			{"update-rate",               required_argument, 0, 'u'},
			{0, 0, 0, 0}
	};

	if (MYTHREAD == 0) {
		int i, c;

		while(1) {
			i = 0;
			c = getopt_long(argc, argv, "hd:i:r:s:u:", long_options, &i);

			if(c == -1)
				break;

			if(c == 0 && long_options[i].flag == 0)
				c = long_options[i].val;

			switch(c) {
			case 0:
				// Flag is automatically set
				break;
			case 'h':
				printf("Usage: %s [-d <int>] [-i <int>] [-r <int>] [-s <int>] [-u <int>]\n", argv[0]);
				exit(0);
			case 'd':
				duration = atoi(optarg);
				break;
			case 'i':
				initial = atoi(optarg);
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
		}
	}
	upc_barrier;

	int nb_threads = THREADS;

	if (seed == 0)
		srand((int) time(0)+MYTHREAD);
	else
		srand(seed+MYTHREAD);

	list_lock = upc_all_lock_alloc();

	/* allocation params */

	int thr_per_node = upcri_pthreads(0);

	int nodes = gasnet_nodes();
	int up, down, pernode;
	up = 0;
	down = 0;
	pernode = initial / nodes;

	if (pernode == 0) {
		pernode = 1;
		nodes = initial;
	}

	if (MYTHREAD == 0) {
		int i, c, val, size;

		struct timeval start, end;
		struct timespec timeout;

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
		printf("int/long/ptr size: %u/%u/%u\n", (unsigned) sizeof(int),
				(unsigned) sizeof(long), (unsigned) sizeof(void*));

		timeout.tv_sec = duration / 1000;
		timeout.tv_nsec = (duration % 1000) * 1000000;

		/* allocate elements and set */
		shared_rbtree = set_new();

		upc_barrier;

		up += pernode;
		for (i = down; i < up; i++) {
			val = (rand() % range) + 1;
			set_add(shared_rbtree, val,0,0);
		}

		/* allocate elements and set */

		upc_barrier;
		size = set_size(shared_rbtree);

		//print_set(shared_intset);

		printf("Set size  : %d\n", size);
		printf("<<<------------->>>\n");
		fflush(stdout);

		upc_barrier;

		printf("STARTING...\n");
		fflush(stdout);

		gettimeofday(&start, NULL);

		if (nanosleep(&timeout, NULL) != 0) {
			perror("nanosleep");
			return 1;
		}

		STOP = 1;

		printf("STOPPING...\n");

		fflush(stdout);

		upc_barrier;

		gettimeofday(&end, NULL);

		duration = (end.tv_sec * 1000 + end.tv_usec / 1000)
						- (start.tv_sec * 1000 + start.tv_usec / 1000);

		upc_barrier;

		int reads = 0;
		int updates = 0;
		int adds = 0;
		int removes = 0;

		for (i = 0; i < nb_threads; i++) {
			/*printf("Thread %d\n", i);
			 printf("  #add      : %d\n", data[i].nb_add);
			 printf("  #remove   : %d\n", data[i].nb_remove);
			 printf("  #contains : %d\n", data[i].nb_contains);
			 printf("  #found    : %d\n", data[i].nb_found);*/
			adds += thread_stat[i].nb_add;
			removes += thread_stat[i].nb_remove;
			updates += (thread_stat[i].nb_add + thread_stat[MYTHREAD].nb_remove);
			reads += thread_stat[i].nb_contains;
			size += thread_stat[i].diff;
		}

		int exp_size = set_size(shared_rbtree);

		printf("Set size    : %d (expected: %d)\n", exp_size, size);
		//  printf("Ads/rms     : (%d / %d)\n", adds, removes);
		printf("Duration    : %d (ms)\n", duration);
		printf("#txs        : %d (%f / s)\n", reads + updates,
				(reads + updates) * 1000.0 / duration);
		printf("#read txs   : %d (%f / s)\n", reads, reads * 1000.0 / duration);
		printf("#update txs : %d (%f / s)\n", updates,
				updates * 1000.0 / duration);
		fflush(stdout);

		upc_barrier;

		set_delete(shared_rbtree);
		upc_lock_free(list_lock);

	} else {
		int i;
		int val;
		upc_barrier;
		// need allocation
		if (MYTHREAD == gasnet_mynode()*thr_per_node){
			down = gasnet_mynode() * pernode;

			if(MYTHREAD == nodes - 1)
				pernode = initial - (nodes -1)*pernode;

			up = down + pernode;
			if (up <= initial + 2) {
				for (i = down; i < up; i++) {
					val = (rand() % range) + 1;
					set_add(shared_rbtree, val, 0, gasnet_mynode());
				}
			}
		}
		upc_barrier;
		test();
                /*printf("MY num %d, node %d, nodes %d\n", MYTHREAD, gasnet_mynode(),gasnet_nodes());
                fflush(stdout);*/
		upc_barrier;
	}

	return 0;
}
