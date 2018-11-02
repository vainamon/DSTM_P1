/*
 * File:
 *   bank.c
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 * Description:
 *   Bank stress test.
 *
 * Copyright (c) 2007-2009.
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
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "../../project/src/dstm_pthread/dstm_pthread.h"
#include "../../project/src/dstm_malloc/dstm_malloc.h"
#include "../../project/src/dstm_barrier.h"

#ifdef DEBUG
# define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define STORE(addr, value) (*(addr) = (value))
#define LOAD(addr) (*(addr))

#define DEFAULT_DURATION                10000
#define DEFAULT_NB_ACCOUNTS             1024
#define DEFAULT_NB_THREADS              144
#define DEFAULT_READ_ALL                10
#define DEFAULT_SEED                    0
#define DEFAULT_WRITE_ALL               30
#define DEFAULT_READ_THREADS            0
#define DEFAULT_WRITE_THREADS           0
#define DEFAULT_NB_NODES                24

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

volatile int STOP = 0;
volatile int THREADS = 0;
barrier_t *loc_b = NULL;
gasnett_mutex_t mtx_b = GASNETT_MUTEX_INITIALIZER;

/* ################################################################### *
: * BANK ACCOUNTS
 * ################################################################### */

typedef struct account {
    int number;
    int balance;
} account_t;

typedef struct bank {
    account_t *accounts;
    int size;
} bank_t;

int transfer(bank_t *bank, unsigned short* seed, int bank_size, int amount)
{
    int i;
     /* Choose random accounts */
    int per_node = bank_size/DEFAULT_NB_NODES;

    int src = (int)(erand48(seed) * per_node);
    int dst = (int)(erand48(seed) * per_node);

    int src_t = (int)(erand48(seed) * (DEFAULT_NB_NODES));
    int dst_t = (int)(erand48(seed) * (DEFAULT_NB_NODES));
    
    if (dst_t == src_t)
      dst_t = (src_t + 1) % DEFAULT_NB_NODES;
//printf("transfer %d %d %d %d\n", src_t, dst_t, src, dst);
//fflush(stdout);
    /* Allow overdrafts */
    __tm_atomic {
        i = (int)LOAD(&bank[src_t].accounts[src].balance);
        i -= amount;
        STORE(&bank[src_t].accounts[src].balance, i);
        i = (int)LOAD(&bank[dst_t].accounts[dst].balance);
        i += amount;
        STORE(&bank[dst_t].accounts[dst].balance, i);
    }
    return amount;
}

int total(bank_t *bank, int bank_size, int transactional)
{
    int i,  j, total;
    int per_node = bank_size/DEFAULT_NB_NODES;
//fprintf(stderr, "in total\n");
    if (!transactional) {
        //    total = 0;
        //    for (i = 0; i < bank->size; i++) {
        //      total += bank->accounts[i].balance;
        //    }
    } else {
        __tm_atomic {
	    total = 0;
//            for (j = 0; j < DEFAULT_NB_NODES; j++) {
// 	        bank_size = LOAD(&bank[j].size);
            for (i = 0; i < per_node; i++) {
		for (j = 0; j < DEFAULT_NB_NODES; j++) {
		   total += (int)LOAD(&bank[j].accounts[i].balance);
                }
            }
        }
    }
//fprintf(stderr, "out total 1\n");

/*FILE* null = fopen("/dev/null", "w");
fprintf(null, "%d\n",total);
fclose(null);*/
//fprintf(stderr, "out total 2\n");

    return total;
}

void reset(bank_t *bank, int bank_size)
{
    int i, j;
    int per_node = bank_size/DEFAULT_NB_NODES;
    __tm_atomic {
//        for (j = 0; j < DEFAULT_NB_NODES; j++) {
//	    bank_size = LOAD(&bank[j].size);
            for (i = 0; i < per_node; i++) {
		for (j = 0; j < DEFAULT_NB_NODES; j++) {
			STORE(&bank[j].accounts[i].balance, 0);
/*STORE(&bank[1].accounts[i].balance, 0);
STORE(&bank[2].accounts[i].balance, 0);
STORE(&bank[3].accounts[i].balance, 0);
STORE(&bank[4].accounts[i].balance, 0);
STORE(&bank[5].accounts[i].balance, 0);
STORE(&bank[6].accounts[i].balance, 0);
STORE(&bank[7].accounts[i].balance, 0);
STORE(&bank[8].accounts[i].balance, 0);
STORE(&bank[9].accounts[i].balance, 0);
STORE(&bank[10].accounts[i].balance, 0);
STORE(&bank[11].accounts[i].balance, 0);
STORE(&bank[12].accounts[i].balance, 0);
STORE(&bank[13].accounts[i].balance, 0);
STORE(&bank[14].accounts[i].balance, 0);
STORE(&bank[15].accounts[i].balance, 0);*/
            }
        }
    }
}

/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data {
    bank_t *bank;
    barrier_t *barrier;
    int barrier_id;
    unsigned long nb_transfer;
    unsigned long nb_read_all;
    unsigned long nb_write_all;
    unsigned int seed;
    int id;
    int read_all;
    int read_threads;
    int write_all;
    int write_threads;
    int tnum;
    int *stop;
    int bank_size;
    char padding[64];
} thread_data_t;

void *test(void *data)
{
    int nb;
    thread_data_t *d = (thread_data_t *)data;
    unsigned short seed[3];

    bank_t *bank;
    //  barrier_t *barrier;
    int barrier_id = 10000;
    unsigned long nb_transfer = 0;
    unsigned long nb_read_all = 0;
    unsigned long nb_write_all = 0;
    unsigned int global_seed;
    int id;
    int read_all = DEFAULT_READ_ALL;
    int read_threads = DEFAULT_READ_THREADS;
    int write_all = DEFAULT_WRITE_ALL;
    int write_threads = DEFAULT_WRITE_THREADS;
    int *pstop;
    int stop = 0;
    int tnum;
    int bank_size;

    __tm_atomic{
        bank = d->bank;
        id = d->id;
        tnum = d->tnum;
        bank_size = d->bank_size;
	global_seed=d->seed;
        //    barrier = d->barrier;
        //    update = d->update;
        //    range = d->range;
        //    tnum = d->tnum;
        //    barrier_id = d->barrier_id;
    }

    THREADS = tnum;

    int SEED = global_seed;

    /* Initialize seed (use rand48 as rand is poor) */
    seed[0] = (unsigned short)rand_r(&SEED);
    seed[1] = (unsigned short)rand_r(&SEED);
    seed[2] = (unsigned short)rand_r(&SEED);

    /* Wait on barrier */
    if(gasnet_mynode() != 0){
        gasnett_mutex_lock(&mtx_b);
        if (loc_b == NULL){
            loc_b = (barrier_t*)malloc(sizeof(barrier_t));
            loc_b->id = barrier_id;
            barrier_init(loc_b, barrier_calculate_threads(tnum));
        }
        gasnett_mutex_unlock(&mtx_b);
        barrier_cross(loc_b,1);
    }else
        barrier_cross(d->barrier,1);

    while (stop == 0) {
        if (id < read_threads) {
            /* Read all */
            total(bank, bank_size, 1);
            nb_read_all++;
        } else if (id < read_threads + write_threads) {
            /* Write all */
            reset(bank, bank_size);
            nb_write_all++;
        } else {
            nb = (int)(erand48(seed) * 100);
            if (nb < read_all) {
                /* Read all */
                total(bank, bank_size, 1);
                nb_read_all++;
            } else if (nb < read_all + write_all) {
               /* Write all */
                reset(bank, bank_size);
                nb_write_all++;
            } else {
                transfer(bank, seed, bank_size, 1);
                nb_transfer++;
            }
        }

        __tm_atomic{
            pstop = d->stop;
            stop = *pstop;
        }
    }

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
        d->nb_read_all = nb_read_all;
        d->nb_write_all = nb_write_all;
        d->nb_transfer = nb_transfer;
   }
}

int main(int argc, char **argv)
{
    /*struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"accounts",                  required_argument, NULL, 'a'},
    {"duration",                  required_argument, NULL, 'd'},
    {"num-threads",               required_argument, NULL, 'n'},
    {"read-all-rate",             required_argument, NULL, 'r'},
    {"read-threads",              required_argument, NULL, 'R'},
    {"seed",                      required_argument, NULL, 's'},
    {"write-all-rate",            required_argument, NULL, 'w'},
    {"write-threads",             required_argument, NULL, 'W'},
    {NULL, 0, NULL, 0}
  };*/

    bank_t *bank;
    int i, j, c;
    unsigned long reads, writes, updates;
    thread_data_t *data;
    dstm_pthread_t *threads;
    dstm_pthread_attr_t attr;
    barrier_t *barrier;
    struct timeval start, end;
    struct timespec timeout;
    int duration = DEFAULT_DURATION;
    int nb_accounts = DEFAULT_NB_ACCOUNTS;
    int nb_threads = DEFAULT_NB_THREADS;
    int read_all = DEFAULT_READ_ALL;
    int read_threads = DEFAULT_READ_THREADS;
    int seed = DEFAULT_SEED;
    int write_all = DEFAULT_WRITE_ALL;
    int write_threads = DEFAULT_WRITE_THREADS;

    int per_nodes_accounts = nb_accounts/DEFAULT_NB_NODES;

    int *stop;

    //  while(1) {
    //    i = 0;
    //    c = getopt_long(argc, argv, "ha:c:d:n:r:R:s:w:W:", long_options, &i);

    //    if(c == -1)
    //      break;

    //    if(c == 0 && long_options[i].flag == 0)
    //      c = long_options[i].val;

    //    switch(c) {
    //     case 0:
    //       /* Flag is automatically set */
    //       break;
    //     case 'h':
    //       printf("bank -- STM stress test\n"
    //              "\n"
    //              "Usage:\n"
    //              "  bank [options...]\n"
    //              "\n"
    //              "Options:\n"
    //              "  -h, --help\n"
    //              "        Print this message\n"
    //              "  -a, --accounts <int>\n"
    //              "        Number of accounts in the bank (default=" XSTR(DEFAULT_NB_ACCOUNTS) ")\n"
    //              "  -d, --duration <int>\n"
    //              "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
    //              "  -n, --num-threads <int>\n"
    //              "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
    //              "  -r, --read-all-rate <int>\n"
    //              "        Percentage of read-all transactions (default=" XSTR(DEFAULT_READ_ALL) ")\n"
    //              "  -R, --read-threads <int>\n"
    //              "        Number of threads issuing only read-all transactions (default=" XSTR(DEFAULT_READ_THREADS) ")\n"
    //              "  -s, --seed <int>\n"
    //              "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
    //              "  -w, --write-all-rate <int>\n"
    //              "        Percentage of write-all transactions (default=" XSTR(DEFAULT_WRITE_ALL) ")\n"
    //              "  -W, --write-threads <int>\n"
    //              "        Number of threads issuing only write-all transactions (default=" XSTR(DEFAULT_WRITE_THREADS) ")\n"
    //         );
    //       exit(0);
    //     case 'a':
    //       nb_accounts = atoi(optarg);
    //       break;
    //     case 'd':
    //       duration = atoi(optarg);
    //       break;
    //     case 'n':
    //       nb_threads = atoi(optarg);
    //       break;
    //     case 'r':
    //       read_all = atoi(optarg);
    //       break;
    //     case 'R':
    //       read_threads = atoi(optarg);
    //       break;
    //     case 's':
    //       seed = atoi(optarg);
    //       break;
    //     case 'w':
    //       write_all = atoi(optarg);
    //       break;
    //     case 'W':
    //       write_threads = atoi(optarg);
    //       break;
    //     case '?':
    //       printf("Use -h or --help for help\n");
    //       exit(0);
    //     default:
    //       exit(1);
    //    }
    //  }

    assert(duration >= 0);
    assert(nb_accounts >= 2);
    assert(nb_threads > 0);
    assert(read_all >= 0 && write_all >= 0 && read_all + write_all <= 100);
    assert(read_threads + write_threads <= nb_threads);

    printf("Nb accounts    : %d\n", nb_accounts);
    printf("Duration       : %d\n", duration);
    printf("Nb threads     : %d\n", nb_threads);
    printf("Read-all rate  : %d\n", read_all);
    printf("Read threads   : %d\n", read_threads);
    printf("Seed           : %d\n", seed);
    printf("Write-all rate : %d\n", write_all);
    printf("Write threads  : %d\n", write_threads);
    printf("Type sizes     : int=%d/long=%d/ptr=%d/word=%d\n",
           (int)sizeof(int),
           (int)sizeof(long),
           (int)sizeof(void *),
           (int)sizeof(uintptr_t));

    timeout.tv_sec = duration / 1000;
    timeout.tv_nsec = (duration % 1000) * 1000000;

    if ((data = (thread_data_t *)dstm_malloc(nb_threads * sizeof(thread_data_t),UINT_MAX)) == NULL) {
        perror("malloc");
        return 1;
    }

    if ((stop = (int *)dstm_malloc(sizeof(int*), UINT_MAX)) == NULL) {
        perror("malloc");
        return 1;
    }

    if ((threads = (dstm_pthread_t *)malloc(nb_threads * sizeof(dstm_pthread_t))) == NULL) {
        perror("malloc");
        return 1;
    }

    if (seed == 0)
        srand((int)time(0));
    else
        srand(seed);
__tm_atomic{
    *stop = 0;
}
    bank = (bank_t *)dstm_malloc(sizeof(bank_t) * DEFAULT_NB_NODES, UINT_MAX);

    for (i = 0; i < DEFAULT_NB_NODES; i++) {
        bank[i].accounts = (account_t *)dstm_malloc(per_nodes_accounts * sizeof(account_t), i);
        bank[i].size = per_nodes_accounts;
	__tm_atomic{
            for (j = 0; j < per_nodes_accounts; j++) {
                STORE(&bank[i].accounts[j].number, j);
                STORE(&bank[i].accounts[j].balance, 0);
            }
        }
    }

    /* Access set from all threads */
    barrier = (barrier_t*)malloc(sizeof(barrier_t));
    barrier->id = 10000;
    barrier_init(barrier,barrier_calculate_threads(nb_threads));

    dstm_pthread_attr_init(&attr);
    dstm_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    printf("<<<------------->>>\n");
    fflush(stdout);

    for (i = 0; i < nb_threads; i++) {
//        printf("Creating thread %d\n", i);
        data[i].id = i;
        data[i].read_all = read_all;
        data[i].read_threads = read_threads;
        data[i].write_all = write_all;
        data[i].write_threads = write_threads;
        data[i].nb_transfer = 0;
        data[i].nb_read_all = 0;
        data[i].nb_write_all = 0;
        data[i].seed = rand();
        data[i].bank = bank;
        data[i].barrier = barrier;
        data[i].tnum = nb_threads;
        data[i].stop = stop;
        data[i].barrier_id = 10000;
        data[i].bank_size = DEFAULT_NB_NODES * per_nodes_accounts;
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
      STORE(stop, 1);
    }
    printf("STOPPING...\n");
    fflush(stdout);

    barrier_cross(barrier,1);

    gettimeofday(&end, NULL);

    /* Wait for thread completion */
    for (i = 0; i < nb_threads; i++) {
      if (dstm_pthread_join(threads[i], NULL) != 0) {
        fprintf(stderr, "Error waiting for thread completion\n");
        return 1;
      }
    }

    duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
    updates = reads = writes = 0;

    for (i = 0; i < nb_threads; i++) {
        /*printf("Thread %d\n", i);
        printf("  #transfer   : %lu\n", data[i].nb_transfer);
        printf("  #read-all   : %lu\n", data[i].nb_read_all);
        printf("  #write-all  : %lu\n", data[i].nb_write_all);*/
        updates += data[i].nb_transfer;
        reads += data[i].nb_read_all;
        writes += data[i].nb_write_all;
    }
    printf("Bank total    : %d (expected: 0)\n", total(bank, DEFAULT_NB_NODES * per_nodes_accounts, 1));
    printf("Duration      : %d (ms)\n", duration);
    printf("#txs          : %lu (%f / s)\n", reads + writes + updates, (reads + writes + updates) * 1000.0 / duration);
    printf("#read txs     : %lu (%f / s)\n", reads, reads * 1000.0 / duration);
    printf("#write txs    : %lu (%f / s)\n", writes, writes * 1000.0 / duration);
    printf("#update txs   : %lu (%f / s)\n", updates, updates * 1000.0 / duration);

    /* Delete bank and accounts */

    free(threads);
    dstm_free(data);

    return 0;
}
