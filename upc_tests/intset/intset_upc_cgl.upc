/*
 ============================================================================
 Name        : intset_upc.upc
 Author      : Danilov Igor
 Version     :
 Copyright   : Your copyright notice
 Description : UPC intset program
 ============================================================================
 */
#include <upc_relaxed.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

#ifdef DEBUG
#define IO_FLUSH                       fflush(NULL)
#endif

//#define DELAY 100

//************************************//

typedef struct node {
	intptr_t val;
	shared struct node *next;
} node_t;

typedef struct intset {
	shared node_t * head;
} intset_t;

upc_lock_t* list_lock;

shared intset_t* shared shared_intset;

//************************************//

shared node_t * new_node(intptr_t val, shared node_t *next, int gasnet_node) {
	shared node_t *node;

	node = (shared node_t *) upc_alloc(sizeof(node_t));
	if (node == NULL) {
		perror("upc_alloc new_node");
		return NULL;
	}

	node->val = val;
	node->next = next;

	return node;
}

shared intset_t * set_new() {
	shared intset_t * set;
	shared node_t *min, *max;

	if ((set = (shared intset_t * ) upc_global_alloc(1,sizeof(intset_t))) == NULL) {
		perror("upc_global_alloc set_new");
		return NULL;
	}
	max = new_node(INT_MAX, NULL, -1);
	min = new_node(INT_MIN, max, -1);
	set->head = min;

	return set;
}

void set_delete(shared intset_t *set) {
	shared node_t *node, *next;

#ifdef DEBUG
	printf("\n++> set_delete\n");
	IO_FLUSH;
#endif
	node = set->head;

	while (node != NULL) {
		next = node->next;
		upc_free(node);
		node = next;
	}
	upc_free(set);
}

void print_set(shared intset_t *set) {
	shared node_t *node, *next;
	int i = 0;
	node = set->head;
	printf("\n-------------\nSet:\n");
	while (node != NULL) {
		next = node->next;
		printf("%d: %p - %d\n", ++i, node, node->val);
		node = next;
		/*next = LOAD(&node->next);*/
	}
	printf("%d: %p - \n----------------\n\n", ++i, node);
	fflush(stdout);
}

int set_size(shared intset_t *set) {
	int size = 0;
	shared node_t *node, *next;

#ifdef DEBUG
	printf("\n++> set_size\n");
	IO_FLUSH;
#endif

	int cycles = 0;
	node = set->head;
	while (node != NULL) {
		size++;
		next = node->next;
		node = next;
	}
	return size;
}

int set_add(shared intset_t *set, intptr_t val, int notx, int gasnet_node) {
	int result;
	shared node_t *prev, *next;
	intptr_t v;
	shared node_t *n = NULL;

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
			prev->next = new_node(val, next, -1);
		}
	} else {
		n = new_node(val, next, gasnet_node);

		upc_lock(list_lock);

		prev = set->head;
		next = prev->next;
		while (1) {
			v = next->val;
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
			next = prev->next;
		}

		result = (v != val);

		if (result) {
			n->val = val;
			n->next = next;
			prev->next = n;
		}

		upc_unlock(list_lock);

		if (!result)
			upc_free(n);
	}

	return result;
}

int set_remove(shared intset_t *set, intptr_t val, int notx) {
	int result;
	shared node_t *prev, *next;
	intptr_t v;
	shared node_t *n;

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
			upc_free(next);
		}
	} else {
		upc_lock(list_lock);

		prev = set->head;
		next = prev->next;

		while (1) {
			v = next->val;
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
			next = prev->next;
		}

		result = (v == val);

		if (result) {
			n = next->next;
			prev->next = n;

			/* Overwrite deleted node */

			next->val = 123456789;
			next->next = next;
		}
	}

	upc_unlock(list_lock);

	/* Free the memory after committed execution */
	if (result)
		upc_free(next);

	return result;
}

int set_contains(shared intset_t *set, intptr_t val, int notx) {
	int result;
	shared node_t *prev, *next;
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

		upc_lock(list_lock);

		prev = set->head;
		next = prev->next;

		while (1) {

			v = (intptr_t) next->val;
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
			next = prev->next;
		}

		result = (v == val);
	}

	upc_unlock(list_lock);

	return result;
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
				if (set_add(shared_intset, val, 0, -1)) {
					diff++;
					last = val;
				}
				nb_add++;
			} else {
				if (set_remove(shared_intset, last, 0))
					diff--;
				nb_remove++;
				last = -1;
			}
		} else {
			val = (rand_r(&SEED) % range) + 1;
			if (set_contains(shared_intset, val, 0))
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
	}

	upc_barrier;

	thread_stat[MYTHREAD].nb_add = nb_add;
	thread_stat[MYTHREAD].nb_remove = nb_remove;
	thread_stat[MYTHREAD].nb_found = nb_found;
	thread_stat[MYTHREAD].nb_contains = nb_contains;
	thread_stat[MYTHREAD].diff = diff;

	/*printf("OUT #%d ads/rms/diff %d/%d/%d \n",MYTHREAD,nb_add,nb_remove,diff);
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
		shared_intset = set_new();

		upc_barrier;

		up += pernode;
		for (i = down; i < up; i++) {
			val = (rand() % range) + 1;
			set_add(shared_intset, val,0,0);
		}

		/* allocate elements and set */

		upc_barrier;
		size = set_size(shared_intset);

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
			 printf("  #add      : %d\n", thread_stat[i].nb_add);
			 printf("  #remove   : %d\n", thread_stat[i].nb_remove);
			 printf("  #contains : %d\n", thread_stat[i].nb_contains);
			 printf("  #found    : %d\n", thread_stat[i].nb_found);*/
			adds += thread_stat[i].nb_add;
			removes += thread_stat[i].nb_remove;
			updates += (thread_stat[i].nb_add + thread_stat[MYTHREAD].nb_remove);
			reads += thread_stat[i].nb_contains;
			size += thread_stat[i].diff;
		}

		int exp_size = set_size(shared_intset);

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

		set_delete(shared_intset);
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
					set_add(shared_intset, val, 0, gasnet_mynode());
				}
			}
		}

		upc_barrier;
		//printf("MY num %d, node %d, nodes %d\n", MYTHREAD, gasnet_mynode(),gasnet_nodes());
		test();
		upc_barrier;
	}

	return 0;
}
