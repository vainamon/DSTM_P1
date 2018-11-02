#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Include the declarations of the STM interface that we need to implement. */
#include "../tanger/include/tanger-stm-internal.h"

#include "dstm_transaction.h"
#include "../profiling.h"

#include <stdint.h>
#include <stdio.h>

#ifdef TRANSACTIONS_STATS

#include "dstm_node_stats.h"

gasnett_mutex_t G_STATS_MUTEX = GASNETT_MUTEX_INITIALIZER;

node_stats_t G_TRANSACTIONS_STATS = {0,0,0,0,0,0,0,0,0,0};

#endif

#define LOAD(bits, type) \
type tanger_stm_load##bits(tanger_stm_tx_t* tx, type *addr) \
{ \
	DSTMTransaction* cur_tx = (DSTMTransaction*)tanger_stm_get_tx(); \
    type value = cur_tx->read<type>(addr);/*return *addr; \*/ \
	return value; \
}

LOAD(8, uint8_t)
//LOAD(16, uint16_t)
LOAD(32, uint32_t)
LOAD(64, uint64_t)
LOAD(16aligned, uint16_t)
LOAD(32aligned, uint32_t)
LOAD(64aligned, uint64_t)

void* tanger_stm_loadregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes)
{
    return addr;
}
void tanger_stm_loadregionpost(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes) {}

#define STORE(bits, type) \
void tanger_stm_store##bits(tanger_stm_tx_t* tx, type *addr, type value) \
{ \
	/*if (debugMsgs > 1)*/ /*fprintf(stderr,"txn=%p store addr=%p\n", tx, (void*)addr);*/ \
	DSTMTransaction* cur_tx = (DSTMTransaction*)tanger_stm_get_tx(); \
	cur_tx->write<type>(addr,value);/*return *addr; \*/ \
}

STORE(8, uint8_t)
//STORE(16, uint16_t)
STORE(32, uint32_t)
STORE(64, uint64_t)
//STORE(16aligned, uint16_t)
STORE(32aligned, uint32_t)
STORE(64aligned, uint64_t)

void* tanger_stm_storeregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes)
{
	//fprintf(stderr,"tanger_stm_storeregionpre\n");
    return addr;
}
void* tanger_stm_updateregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes)
{
	//fprintf(stderr,"tanger_stm_updateregionpre\n");
    return addr;
}


/**
 * Starts or restarts a new transaction.
 */
uint32_t ITM_REGPARM _ITM_beginTransaction(uint32_t properties,...)
{
//	fprintf(stderr,"_ITM_beginTransaction\n");

	tanger_stm_tx_t* tx = tanger_stm_get_tx();
	//fprintf(stderr,"txn=%p start id = %d, status = %d\n", tx, ((stm_tx_t*)tx)->threadID,((stm_tx_t*)tx)->status);
	((DSTMTransaction*)tx)->begin();
	return 0x01; /* a_runUninstrumentedCode     = 0x02 */
}

/**
 * Tries to commit a transaction.
 * We can ignore longjmp, and we always commit successfully.
 */
void ITM_REGPARM _ITM_commitTransaction()
{
//	fprintf(stderr,"_ITM_commitTransaction\n");

	DSTMTransaction* tx = (DSTMTransaction*)tanger_stm_get_tx();
	tx->commit();
 //   fprintf(stderr,"txn=%p commit\n", tx);

#ifdef TRANSACTIONS_STATS
	transaction_stats_t stats = tx->getStats();
	gasnett_mutex_lock(&G_STATS_MUTEX);

	G_TRANSACTIONS_STATS.total_txs++;
	G_TRANSACTIONS_STATS.total_txs_stats = G_TRANSACTIONS_STATS.total_txs_stats + stats;

	gasnett_mutex_unlock(&G_STATS_MUTEX);
#endif

	gasnett_threadkey_set(TX_DATA_THREADKEY, NULL);

	delete tx;

}

bool ITM_REGPARM _ITM_tryCommitTransaction()
{
    /* we can always commit */
	fprintf(stderr,"_ITM_tryCommitTransaction\n");
    return 1;
}


/**
 * Per-thread initialization.
 */
static void tanger_stm_thread_init() __attribute__((noinline));
static void tanger_stm_thread_init()
{
	//fprintf(stderr,"_tanger_stm_thread_init\n");
//	stm_tx_t* tx;
//
//    pthread_mutex_lock(&globalLock);
//
//    /* set transaction data */
//    if ((tx = (stm_tx_t*)malloc(sizeof(stm_tx_t))) == NULL) {
//        perror("malloc");
//        exit(1);
//    }
//    tx->threadID = threadCounter;
//    tx->status = TX_IDLE;
//    gasnett_threadkey_set(TX_DATA_THREADKEY, tx);
//    threadCounter++;
//    pthread_mutex_unlock(&globalLock);
}


/**
 * Returns the calling thread's transaction descriptor.
 * TODO specify when the function is supposed to return a valid value (e.g.,
 * only in transactional code or always)
 */
tanger_stm_tx_t* tanger_stm_get_tx()
{
//	fprintf(stderr,"tanger_stm_get_tx\n");
    tanger_stm_tx_t* tx = gasnett_threadkey_get(TX_DATA_THREADKEY);
    if(tx == NULL){
//        fprintf(stderr,"tanger_stm_get_tx %p\n",tx);
    	tx = new DSTMTransaction();
    	gasnett_threadkey_set(TX_DATA_THREADKEY, tx);
    }
//    while (!(tx = (tanger_stm_tx_t*)pthread_getspecific(threadLocalKey)))
//        tanger_stm_thread_init();
    return tx;
}


/**
 * Saves or restores the stack, depending on whether the current txn was
 * started or restarted. The STM will save/restore everything in the range
 * [low_addr, high_addr). The STM's implementation of this function must be
 * marked as no-inline, so it will get a new stack frame that does not
 * overlap the [low_addr, high_addr) region.
 * To avoid corrupting stack space of rollback functions, the STM should skip
 * undoing changes to addresses that are between the current stack pointer
 * during execution of the undo function and the [low_addr, high_addr)
 * area (i.e., all newer stack frames, including the current one).
 */
void tanger_stm_save_restore_stack(void* low_addr, void* high_addr)
{
    tanger_stm_tx_t* tx = gasnett_threadkey_get(TX_DATA_THREADKEY);
    if(tx == NULL){
    	tx = new DSTMTransaction();
    	gasnett_threadkey_set(TX_DATA_THREADKEY, tx);
    }
//	fprintf(stderr,"tanger_stm_save_restore_stack tx %p\n",tx);
    ((DSTMTransaction*)tx)->saveCheckpoint();
}

/**
 * Must be called by the thread after all transactions have finished.
 * FIXME this is never called. DummySTM should garbage-collect txns.
 */
#ifdef __cplusplus
extern "C" {
#endif

void ITM_REGPARM tanger_stm_thread_shutdown()
{
	DSTMTransaction* tx = (DSTMTransaction*)gasnett_threadkey_get(TX_DATA_THREADKEY);

	//fprintf(stderr,"txn=%p thread_shutdown id = %d, status = %d\n", tx, ((stm_tx_t*)tx)->threadID,((stm_tx_t*)tx)->status);

	if (tx != NULL){
		gasnett_threadkey_set(TX_DATA_THREADKEY, NULL);
		fprintf(stderr,"thread_shutdown del tx\n");
		delete tx;
	}
}

#ifdef __cplusplus
}
#endif

static void _ITM_finalizeProcessCCallConv() { _ITM_finalizeProcess(); }

/**
 * Called after Tanger's STM support system has be initialized.
 */
int ITM_REGPARM _ITM_initializeProcess()
{
#ifdef LOGGING
	VLOG(0)<<"_ITM_initializeProcess "<<"node "<<gasnet_mynode()<<"\n";
#endif

	initCache();

	initMemory();

//    pthread_mutexattr_t attr;
//    int result = pthread_key_create(&threadLocalKey, NULL);
//    if (result != 0) {
//        perror("creating thread-local key");
//        exit(1);
//    }
//	result = atexit(_ITM_finalizeProcessCCallConv);
//    if (result != 0) {
//        perror("registering atexit function");
//        exit(1);
//    }
//    pthread_mutexattr_init(&attr);
//    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
//    pthread_mutex_init(&globalLock, &attr);
    return 0;
}

/**
 * Called before Tanger's STM support system is shut down.
 */
void ITM_REGPARM _ITM_finalizeProcess()
{
#ifdef LOGGING
	VLOG(0)<<"_ITM_finalizeProcess "<<"node "<<gasnet_mynode()<<"\n";
#endif

	dstm_p1_lib_shutdown();

#ifdef TRANSACTIONS_STATS
	glue::sendNodeStats(G_TRANSACTIONS_STATS);
#endif
}

/**
 * Switches to another STM mode (e.g., serial-irrevocable).
 * We already have the global lock, and we don't support any other mode.
 */
void ITM_REGPARM _ITM_changeTransactionMode(_ITM_transactionState mode) {}


/**
 * Replacement function for malloc calls in transactions.
 */
void *tanger_stm_malloc(size_t size)
{
	return malloc(size);
}

/**
 * Replacement function for free calls in transactions.
 */
void tanger_stm_free(void *ptr)
{
	free(ptr);
}

/**
 * Replacement function for calloc calls in transactions.
 */
void *tanger_stm_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}

/**
 * Replacement function for realloc calls in transactions.
 */
void *tanger_stm_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void tanger_stm_indirect_init(uint32_t number_of_call_targets)
{

}

void tanger_stm_indirect_init_multiple(uint32_t number_of_call_targets)
{

}

void tanger_stm_indirect_register_multiple(void* nontxnal, void* txnal)
{

}

void tanger_stm_indirect_register(void* nontxnal, void* txnal)
{

}

void* tanger_stm_indirect_resolve_multiple(void *nontxnal_function){
	return nontxnal_function;
}
