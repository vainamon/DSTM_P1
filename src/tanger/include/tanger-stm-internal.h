/* Copyright (C) 2007-2009  Torvald Riegel
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
/**
 * Internal STM interface. Contains the interface that an STM has to implement
 * for Tanger. The part of the interface that is visible in the application is
 * in tanger-stm.h.
 */
#ifndef TANGERSTMINTERNAL_H_
#define TANGERSTMINTERNAL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A transaction descriptor/handle/... */
typedef void tanger_stm_tx_t;

#ifndef TANGER_LOADSTORE_ATTR
#define TANGER_LOADSTORE_ATTR __attribute__((nothrow,always_inline))
#endif

#if defined(__i386__)
/* XXX This is not supported by LLVM yet. */
#define ITM_REGPARM __attribute__((regparm(2)))
#else
#define ITM_REGPARM
#endif

/* Load and store functions access a certain number of bits.
 * 1b loads/stores are currently assumed to actually go to full 8 bits.
 * Addresses being accessed are not necessarily aligned (e.g., a 16b load
 * might target memory address 1). If it is known at compile time that the
 * access is aligned, then different functions are called.
 */
uint8_t tanger_stm_load1(tanger_stm_tx_t* tx, uint8_t *addr) TANGER_LOADSTORE_ATTR;
uint8_t tanger_stm_load8(tanger_stm_tx_t* tx, uint8_t *addr) TANGER_LOADSTORE_ATTR;
uint16_t tanger_stm_load16(tanger_stm_tx_t* tx, uint16_t *addr) TANGER_LOADSTORE_ATTR;
uint32_t tanger_stm_load32(tanger_stm_tx_t* tx, uint32_t *addr) TANGER_LOADSTORE_ATTR;
uint64_t tanger_stm_load64(tanger_stm_tx_t* tx, uint64_t *addr) TANGER_LOADSTORE_ATTR;
uint16_t tanger_stm_load16aligned(tanger_stm_tx_t* tx, uint16_t *addr) TANGER_LOADSTORE_ATTR;
uint32_t tanger_stm_load32aligned(tanger_stm_tx_t* tx, uint32_t *addr) TANGER_LOADSTORE_ATTR;
uint64_t tanger_stm_load64aligned(tanger_stm_tx_t* tx, uint64_t *addr) TANGER_LOADSTORE_ATTR;

/** Loads a number of bytes from src and copies them to dest
 * src is shared data, dest must point to thread-private data */
void tanger_stm_loadregion(tanger_stm_tx_t* tx, uint8_t *src, uintptr_t bytes, uint8_t *dest);
/** Starts reading a number of bytes from addr.
 * Mostly useful for creating wrappers for library functions. Use with care!
 * The function returns an address that you can read the data from. Depending
 * on the STM algorithm, it might be different from addr or not.
 * You must call tanger_stm_loadregionpost after calling this function.
 * You must not call any other STM function between the pre and post calls.
 * Data read from addr between the two calls is not guaranteed to be a
 * consistent snapshot. */
void* tanger_stm_loadregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes);
/** See tanger_stm_loadregionpre */
void tanger_stm_loadregionpost(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes);
/** This is like tanger_stm_loadregionpre() except that it assumes that addr
 * points to a zero-terminated string and thus reads all bytes up to and
 * including the final zero. It returns the size of the string (including the
 * terminating zero) in bytes. The size is guaranteed to be derived from a
 * consistent snapshot. If you read the strings' contents, you must call
 * tanger_stm_loadregionpost() afterwards.
 */
void* tanger_stm_loadregionstring(tanger_stm_tx_t* tx, char *addr, uintptr_t *bytes);

void tanger_stm_store1(tanger_stm_tx_t* tx, uint8_t *addr, uint8_t value) TANGER_LOADSTORE_ATTR;
void tanger_stm_store8(tanger_stm_tx_t* tx, uint8_t *addr, uint8_t value) TANGER_LOADSTORE_ATTR;
void tanger_stm_store16(tanger_stm_tx_t* tx, uint16_t *addr, uint16_t value) TANGER_LOADSTORE_ATTR;
void tanger_stm_store32(tanger_stm_tx_t* tx, uint32_t *addr, uint32_t value) TANGER_LOADSTORE_ATTR;
void tanger_stm_store64(tanger_stm_tx_t* tx, uint64_t *addr, uint64_t value) TANGER_LOADSTORE_ATTR;
void tanger_stm_store16aligned(tanger_stm_tx_t* tx, uint16_t *addr, uint16_t value) TANGER_LOADSTORE_ATTR;
void tanger_stm_store32aligned(tanger_stm_tx_t* tx, uint32_t *addr, uint32_t value) TANGER_LOADSTORE_ATTR;
void tanger_stm_store64aligned(tanger_stm_tx_t* tx, uint64_t *addr, uint64_t value) TANGER_LOADSTORE_ATTR;
/** Reads a number of bytes from src and writes them to dest
 * dest is shared data, src must point to thread-private data */
void tanger_stm_storeregion(tanger_stm_tx_t* tx, uint8_t *src, uintptr_t bytes, uint8_t *dest);
/** Prepares writing a number of bytes to addr.
 * The function returns an address that you can write the data to. Depending
 * on the STM algorithm, it might be different from addr or not.
 * The memory starting at addr does not necessarily contain a consistent
 * snapshot or the data previously located at this memory region. */
void* tanger_stm_storeregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes);
/** Prepares updating a number of bytes starting at addr.
 * The function returns an address that you can write the data to. Depending
 * on the STM algorithm, it might be different from addr or not.
 * The memory starting at the returned address will contain a consistent
 * snapshot of the previous values of the region. */
void* tanger_stm_updateregionpre(tanger_stm_tx_t* tx, uint8_t *addr, uintptr_t bytes);


/*
 * The following functions/types are documented in the ABI.
 */
#define _ITM_VERSION "0.90 (Feb 29 2008)"
#define _ITM_VERSION_NO 90
int _ITM_versionCompatible(int) ITM_REGPARM;
const char* _ITM_libraryVersion(void) ITM_REGPARM;

typedef enum {
    pr_instrumentedCode = 0x0001,
    pr_uninstrumentedCode = 0x0002,
    pr_multiwayCode = pr_instrumentedCode | pr_uninstrumentedCode,
    /* GCC extension:  */
    pr_hasNoVectorUpdate = 0x0004,
    pr_hasNoAbort = 0x0008,
    /* GCC extension:  */
    pr_hasNoFloatUpdate = 0x0010,
    pr_hasNoIrrevocable = 0x0020,
    pr_doesGoIrrevocable = 0x0040,
    pr_aWBarriersOmitted = 0x0100,
    pr_RaRBarriersOmitted = 0x0200,
    pr_undoLogCode = 0x0400,
    pr_preferUninstrumented = 0x0800,
    pr_exceptionBlock = 0x1000,
    pr_readOnly = 0x4000,
    pr_hasElse = 0x200000,
    pr_hasNoSimpleReads = 0x400000
} _ITM_codeProperties;

typedef enum {
    a_runInstrumentedCode = 0x01,
    a_runUninstrumentedCode = 0x02,
    a_saveLiveVariables = 0x04,
    a_restoreLiveVariables = 0x08,
    a_abortTransaction = 0x10
} _ITM_actions;

extern uint32_t _ITM_beginTransaction(uint32_t, ...) ITM_REGPARM;
extern void _ITM_commitTransaction(void) ITM_REGPARM;
extern bool _ITM_tryCommitTransaction(void) ITM_REGPARM;

typedef enum {
    userAbort = 1,
    userRetry = 2,
    TMConflict = 4,
    exceptionBlockAbort = 8,
    outerAbort = 16
} _ITM_abortReason;
extern void _ITM_abortTransaction(_ITM_abortReason) ITM_REGPARM  __attribute__((noreturn));

typedef enum {
    modeSerialIrrevocable
} _ITM_transactionState;
void _ITM_changeTransactionMode(_ITM_transactionState) ITM_REGPARM;

typedef enum {
    outsideTransaction = 0,
    inRetryableTransaction,
    inIrrevocableTransaction
} _ITM_howExecuting;
_ITM_howExecuting _ITM_inTransaction(void) ITM_REGPARM;

#define _ITM_noTransactionID 1 /* ID for nontransactional code */

/*
 * Process init/shutdown functions do not need to be called by the compiler.
 * However, they must be idempotent, so they can be called.
 */
extern int _ITM_initializeProcess(void) ITM_REGPARM;
extern void _ITM_finalizeProcess(void) ITM_REGPARM;

void _ITM_error(const void*, int) ITM_REGPARM __attribute__((noreturn));

/**
 * Returns the calling thread's transaction descriptor.
 * ABI note: Remove this once we have efficient TLS.
 */
tanger_stm_tx_t* tanger_stm_get_tx(void);

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
void tanger_stm_save_restore_stack(void* low_addr, void* high_addr);

/**
 * Replacement function for malloc calls in transactions.
 */
void *tanger_stm_malloc(size_t size);

/**
 * Replacement function for free calls in transactions.
 */
void tanger_stm_free(void *ptr);

/**
 * Replacement function for calloc calls in transactions.
 */
void *tanger_stm_calloc(size_t nmemb, size_t size);

/**
 * Replacement function for realloc calls in transactions.
 */
void *tanger_stm_realloc(void *ptr, size_t size);

/**
 * Returns the transactional version of the function passed as argument.
 * If no transactional version has been registered, it aborts.
 */
void* tanger_stm_indirect_resolve(void *nontxnal_function);

/**
 * Called before transactional versions are registered for nontransactional
 * functions.
 * The parameter returns the exact number of functions that will be registered.
 */
void tanger_stm_indirect_init(uint32_t number_of_call_targets);

/**
 * Registers a transactional versions for a nontransactional function.
 */
void tanger_stm_indirect_register(void* nontxnal, void* txnal);

void tanger_stm_indirect_init_multiple(uint32_t number_of_call_targets);
void tanger_stm_indirect_register_multiple(void* nontxnal, void* txnal);
void* tanger_stm_indirect_resolve_multiple(void *nontxnal_function);
#ifdef __cplusplus
}
#endif

#endif /*TANGERSTMINTERNAL_H_*/
