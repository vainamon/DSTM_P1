#ifndef DSTM_LIB_P1_H
#define DSTM_LIB_P1_H

#include <gasnet.h>
#include <gasnet_tools.h>

#include "gasnet_mem_mgr.h"
#include "../dstm_cache.h"

extern DSTMGASNetCache *globalCache;

extern GASNetMemoryManager *globalMemMgr;

GASNETI_THREADKEY_DECLARE(TX_DATA_THREADKEY);

#ifdef __cplusplus
extern "C" {
#endif

void initCache();

void initMemory();

void dstm_p1_lib_shutdown();

#ifdef __cplusplus
}
#endif

#endif
