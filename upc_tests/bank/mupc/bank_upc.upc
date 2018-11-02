/*
 ============================================================================
 Name        : bank_upc.upc
 Author      : Danilov Igor
 Version     :
 Copyright   : Your copyright notice
 Description : UPC Hello world program
 ============================================================================
 */
#include <upc_relaxed.h>

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#ifdef DEBUG
# define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define DEFAULT_DURATION                1000
#define DEFAULT_NB_ACCOUNTS             1024
#define DEFAULT_NB_THREADS              1
#define DEFAULT_READ_ALL                30
#define DEFAULT_SEED                    0
#define DEFAULT_WRITE_ALL               10
#define DEFAULT_READ_THREADS            0
#define DEFAULT_WRITE_THREADS           0

#define PER_THREAD_ACCOUNTS             22

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

shared int STOP;

upc_lock_t* bank_lock;

/* ################################################################### *
 * BANK ACCOUNTS
 * ################################################################### */

typedef struct account {
	int number;
	int balance;
} account_t;

//typedef struct bank {
	shared [PER_THREAD_ACCOUNTS] account_t shared_bank[PER_THREAD_ACCOUNTS*THREADS];
//	shared int size;
//} bank_t;

//shared bank_t shared_bank;

int transfer(shared [PER_THREAD_ACCOUNTS] account_t *src, shared [PER_THREAD_ACCOUNTS] account_t *dst, int amount)
{
	int i;
	/* Allow overdrafts */
	upc_lock(bank_lock);
	i = (int)(src->balance);
	i -= amount;
	src->balance = i;
	i = (int)(dst->balance);
	i += amount;
	dst->balance = i;
	upc_unlock(bank_lock);

	return amount;
}

int total(int transactional)
{
	int i, total;

	if (!transactional) {
		total = 0;
		for (i = 0; i < DEFAULT_NB_ACCOUNTS; i++) {
			total += shared_bank[i].balance;
		}
	} else {
		upc_lock(bank_lock);
		total = 0;
		for (i = 0; i < DEFAULT_NB_ACCOUNTS; i++) {
			total += (int)(shared_bank[i].balance);
		}
		upc_unlock(bank_lock);
	}

	return total;
}

void reset()
{
	int i;

	upc_lock(bank_lock);
	for (i = 0; i < DEFAULT_NB_ACCOUNTS; i++) {
		shared_bank[i].balance = 0;
	}
	upc_unlock(bank_lock);
}

/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data {
	unsigned long nb_transfer;
	unsigned long nb_read_all;
	unsigned long nb_write_all;
} thread_data_t;

shared thread_data_t thread_stat[THREADS];

shared unsigned int global_seed;
shared int read_all;
shared int read_threads;
shared int write_all;
shared int write_threads;

void test()
{
	int src, dst, nb;
	unsigned short seed[3];
	unsigned int g_seed = 0;
        if (g_seed == 0)
                srand((int)time(0) + MYTHREAD);
        else            
                srand(g_seed + MYTHREAD);

	/* Initialize seed (use rand48 as rand is poor) */
	seed[0] = (unsigned short)rand_r(&g_seed);
	seed[1] = (unsigned short)rand_r(&g_seed);
	seed[2] = (unsigned short)rand_r(&g_seed);
	/* Wait on barrier */
	upc_barrier;
  
	while (STOP == 0) {
		if (MYTHREAD < read_threads) {
			/* Read all */
			total(1);
			thread_stat[MYTHREAD].nb_read_all++;
		} else if (MYTHREAD < read_threads + write_threads) {
			/* Write all */
			reset();
			thread_stat[MYTHREAD].nb_write_all++;
		} else {
			nb = (int)((double)erand48(seed) * (double)100);
			nb = (int)(rand_r(&g_seed) % 100);
			if (nb < read_all) {
				/* Read all */
				total(1);
				thread_stat[MYTHREAD].nb_read_all++;
			} else if (nb < read_all + write_all) {
				/* Write all */
				reset();
				thread_stat[MYTHREAD].nb_write_all++;
			} else {
				/* Choose random accounts */
				/*src = (int)(erand48(seed) * DEFAULT_NB_ACCOUNTS);
				dst = (int)(erand48(seed) * DEFAULT_NB_ACCOUNTS);*/
				src = (int)(rand_r(&g_seed) % DEFAULT_NB_ACCOUNTS);
				dst = (int)(rand_r(&g_seed) % DEFAULT_NB_ACCOUNTS);
				if (dst == src)
					dst = (src + 1) % DEFAULT_NB_ACCOUNTS;
				//printf("transfer %d %d\n", src, dst);
				transfer(&shared_bank[src], &shared_bank[dst], 1);
				thread_stat[MYTHREAD].nb_transfer++;
			}
		}
	}
}

int main(int argc, char **argv)
{
	struct option long_options[] = {
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
	};

	unsigned long reads, writes, updates;

	struct timeval start, end;
	struct timespec timeout;

	int duration = DEFAULT_DURATION;
	int nb_accounts = DEFAULT_NB_ACCOUNTS;
	int nb_threads = THREADS;

	read_all = DEFAULT_READ_ALL;
	read_threads = DEFAULT_READ_THREADS;
	write_all = DEFAULT_WRITE_ALL;
	write_threads = DEFAULT_WRITE_THREADS;

	if (MYTHREAD == 0) {
		int i, c;

		while(1) {
			i = 0;
			c = getopt_long(argc, argv, "ha:c:d:n:r:R:s:w:W:", long_options, &i);

			if(c == -1)
				break;

			if(c == 0 && long_options[i].flag == 0)
				c = long_options[i].val;

			switch(c) {
			case 0:
				/* Flag is automatically set */
				break;
			case 'h':
				printf("bank -- STM stress test\n"
						"\n"
						"Usage:\n"
						"  bank [options...]\n"
						"\n"
						"Options:\n"
						"  -h, --help\n"
						"        Print this message\n"
						"  -a, --accounts <int>\n"
						"        Number of accounts in the bank (default=" XSTR(DEFAULT_NB_ACCOUNTS) ")\n"
						"  -d, --duration <int>\n"
						"        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
						"  -n, --num-threads <int>\n"
						"        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
						"  -r, --read-all-rate <int>\n"
						"        Percentage of read-all transactions (default=" XSTR(DEFAULT_READ_ALL) ")\n"
						"  -R, --read-threads <int>\n"
						"        Number of threads issuing only read-all transactions (default=" XSTR(DEFAULT_READ_THREADS) ")\n"
						"  -s, --seed <int>\n"
						"        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
						"  -w, --write-all-rate <int>\n"
						"        Percentage of write-all transactions (default=" XSTR(DEFAULT_WRITE_ALL) ")\n"
						"  -W, --write-threads <int>\n"
						"        Number of threads issuing only write-all transactions (default=" XSTR(DEFAULT_WRITE_THREADS) ")\n"
				);
				exit(0);
			case 'a':
				nb_accounts = atoi(optarg);
				break;
			case 'd':
				duration = atoi(optarg);
				break;
			case 'n':
				nb_threads = atoi(optarg);
				break;
			case 'r':
				read_all = atoi(optarg);
				break;
			case 'R':
				read_threads = atoi(optarg);
				break;
			case 's':
				global_seed = atoi(optarg);
				break;
			case 'w':
				write_all = atoi(optarg);
				break;
			case 'W':
				write_threads = atoi(optarg);
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

	if (global_seed == 0)
		srand((int)time(0) + MYTHREAD);
	else
		srand(global_seed + MYTHREAD);

	bank_lock = upc_all_lock_alloc();

	if (MYTHREAD == 0)
	{
		int i;
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
		printf("Seed           : %d\n", global_seed);
		printf("Write-all rate : %d\n", write_all);
		printf("Write threads  : %d\n", write_threads);
		printf("Type sizes     : int=%d/long=%d/ptr=%d/word=%d\n",
				(int)sizeof(int),
				(int)sizeof(long),
				(int)sizeof(void *),
				(int)sizeof(uintptr_t));

		timeout.tv_sec = duration / 1000;
		timeout.tv_nsec = (duration % 1000) * 1000000;

		for (i = 0; i < DEFAULT_NB_ACCOUNTS; i++) {
			shared_bank[i].number = i;
			shared_bank[i].balance = 0;
		}

		STOP = 0;

		for (i = 0; i < nb_threads; i++) {
			thread_stat[i].nb_transfer = 0;
			thread_stat[i].nb_read_all = 0;
			thread_stat[i].nb_write_all = 0;
		}
		
		upc_barrier;
		printf("STARTING...\n");
		fflush(stdout);
		gettimeofday(&start, NULL);

		if (duration > 0)
			nanosleep(&timeout, NULL);
		
		STOP = 1;
		gettimeofday(&end, NULL);
		printf("STOPPING...\n");

		upc_barrier;

		duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
		updates = reads = writes = 0;
	
		for (i = 0; i < nb_threads; i++) {
			/*printf("Thread %d\n", i);
			printf("  #transfer   : %lu\n", thread_stat[i].nb_transfer);
			printf("  #read-all   : %lu\n", thread_stat[i].nb_read_all);
			printf("  #write-all  : %lu\n", thread_stat[i].nb_write_all);*/
			updates += thread_stat[i].nb_transfer;
			reads += thread_stat[i].nb_read_all;
			writes += thread_stat[i].nb_write_all;
		}

		printf("Bank total    : %d (expected: 0)\n", total(0));
		printf("Duration      : %d (ms)\n", duration);
		printf("#txs          : %lu (%f / s)\n", reads + writes + updates, (reads + writes + updates) * 1000.0 / duration);
		printf("#read txs     : %lu (%f / s)\n", reads, reads * 1000.0 / duration);
		printf("#write txs    : %lu (%f / s)\n", writes, writes * 1000.0 / duration);
		printf("#update txs   : %lu (%f / s)\n", updates, updates * 1000.0 / duration);

		upc_lock_free(bank_lock);
	} else {
		test();
		upc_barrier;
	}

	return 0;
}

