/*
 * gasnet_handlers.h
 *
 *  Created on: 12.09.2011
 *      Author: igor
 */

#ifndef GASNET_HANDLERS_H_
#define GASNET_HANDLERS_H_

#include "gasnet_common.h"
#include "application.h"

int gasnet_handlers_init(Application* __application);

#define hidx_endexec_shrthandler				201
#define hidx_endexec_succ_shrthandler			202
#define hidx_spawnthread_medhandler				203
#define hidx_spawnthread_succ_medhandler		204
#define hidx_threadexited_shrthandler			205
#define hidx_threadexited_succ_shrthandler		206
#define hidx_requestlocks_medhandler			207
#define hidx_requestlocks_succ_medhandler		208
#define hidx_dstmfree_medhandler				209
#define hidx_dstmfree_succ_medhandler			210
#define hidx_dstmmalloc_shrthandler				211
#define hidx_dstmmalloc_succ_shrthandler		212
#define hidx_dstmstats_medhandler				213
#define hidx_dstmstats_succ_medhandler			214

typedef gasnet_handlerarg_t harg_t;

void    endexec_shrthandler(gasnet_token_t token);
void    endexec_succ_shrthandler(gasnet_token_t token);

void    spawnthread_medhandler(gasnet_token_t, void*, size_t, harg_t);
void    spawnthread_succ_medhandler(gasnet_token_t, void*, size_t, harg_t);

void    threadexited_shrthandler(gasnet_token_t token, harg_t gtid, harg_t state);
void    threadexited_succ_shrthandler(gasnet_token_t token, harg_t gtid, harg_t state);

void    requestlocks_medhandler(gasnet_token_t, void*, size_t);
void    requestlocks_succ_medhandler(gasnet_token_t, void*, size_t, harg_t);

void    dstmfree_medhandler(gasnet_token_t, void*, size_t);
void    dstmfree_succ_medhandler(gasnet_token_t, void*, size_t);

void    dstmmalloc_shrthandler(gasnet_token_t, harg_t, harg_t, harg_t, harg_t, harg_t);
void    dstmmalloc_succ_shrthandler(gasnet_token_t, harg_t, harg_t, harg_t,harg_t, harg_t, harg_t);

void    dstmstats_medhandler(gasnet_token_t, void*, size_t);
void    dstmstats_succ_medhandler(gasnet_token_t, void*, size_t);

/*
 *  GASNet handler's table
 */

extern gasnet_handlerentry_t htable[14];

extern int GLOBAL_END_EXECUTION;

#ifdef TRANSACTIONS_STATS
extern volatile int GLOBAL_RETRIVE_STATS;
#endif

#define HANDLER_TABLE_SIZE (sizeof(htable)/sizeof(gasnet_handlerentry_t))

#endif /* GASNET_HANDLERS_H_ */
