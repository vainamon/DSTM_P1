/*
 * glue.cpp
 *
 *  Created on: 28.02.2012
 *      Author: igor
 */

#include "glue.h"

#include "gasnet_handlers.h"

#include "dstm_lib_p1/dstm_transaction.h"

#include "dstm_lib_p1/memory_manager.h"

#include "dstm_lib_p1/dstm_timestamps.h"

#include <gasnet_handler.h>

#include <vector>

MemoryManager* MEMORY_MANAGER;

void glue::setMemoryManager(uintptr_t mem_mgr)
{
    MEMORY_MANAGER = (GASNetMemoryManager*)mem_mgr;
}

void glue::requestLocks(uint64_t *objects_versions, size_t nbytes, uint32_t node)
{
    GASNET_Safe(gasnet_AMRequestMedium0
                (node, hidx_requestlocks_medhandler, objects_versions, nbytes));
}

uint64_t* glue::processLocksRequest(uint64_t *objects_versions, size_t nbytes)
{
    int objectsNum = (nbytes - 2*sizeof(uint64_t))/16;

    int realObjectsNum = objects_versions[1];

    int currentObject = 0;

    std::vector<uint64_t> changedObjs;

    while (currentObject < realObjectsNum) {
        uintptr_t obj_addr = objects_versions[currentObject+2];

        uint64_t *lock = (uint64_t*)MEMORY_MANAGER->getObjectLockAddr(obj_addr, gasnet_mynode());

        uint64_t lock_val_old = *lock;

        /// set lock to transaction descriptor
        /// uint64_t lock_val_new = (objects_versions[0] | ((uint64_t)1<<63));
        uint64_t lock_val_new = (objects_versions[objectsNum + currentObject + 2] | ((uint64_t)1<<63));

        uint64_t current_version = objects_versions[objectsNum + currentObject + 2];

        // TODO: Contention management

        if ((lock_val_old > current_version)
                || (!gasnett_atomic64_compare_and_swap((gasnett_atomic64_t*)lock, lock_val_old, lock_val_new, GASNETT_ATOMIC_RMB_PRE | GASNETT_ATOMIC_RMB_POST))){
            changedObjs.push_back(obj_addr | TO_NODE_TS(gasnet_mynode()));
        }

        currentObject++;
    }

    if (!changedObjs.empty()){

        uint64_t* ch_objs = (uint64_t*)malloc((changedObjs.size()+2)*sizeof(uint64_t));

        ch_objs[1] = changedObjs.size();

        memcpy(ch_objs + 2, &changedObjs.front(), changedObjs.size()*sizeof(uint64_t));

        return ch_objs;
    }else
        return NULL;
}

void glue::locksRequestProcessed(uintptr_t tx, uint64_t* chandedObjs, uint32_t nobjs)
{
    ((Transaction*)tx)->locksRequestProcessed(chandedObjs, nobjs);
}

gasnett_atomic64_t LOCAL_CLOCK = gasnett_atomic64_init(0);

uint64_t glue::getCurrentClock()
{
    return gasnett_atomic64_read(&LOCAL_CLOCK, GASNETT_ATOMIC_RMB_PRE | GASNETT_ATOMIC_RMB_POST);
}

bool glue::increaseClock(uint64_t new_clock)
{
    int tries = 0;

    while (1)
    {
        uint64_t cur_clock = getCurrentClock();

        int64_t delta = new_clock - cur_clock;

        if (delta > 0){
            tries++;
            if (gasnett_atomic64_compare_and_swap(&LOCAL_CLOCK, cur_clock,
                                                  cur_clock + delta, GASNETT_ATOMIC_RMB_PRE | GASNETT_ATOMIC_RMB_POST)){
                return true;
            }
        } else {
            if (tries > 0)
                return true;
            else
                return false;
        }
    }
}

uint64_t glue::getNextClock()
{
    while (1) {
        uint64_t cur_clock = getCurrentClock();

        if (gasnett_atomic64_compare_and_swap(&LOCAL_CLOCK, cur_clock,
                                              cur_clock + 1, GASNETT_ATOMIC_RMB_PRE | GASNETT_ATOMIC_RMB_POST))
            return cur_clock + 1;
    }

    return getCurrentClock();
}

void glue::dstm_mallocFreeRequest(uint32_t _node, uintptr_t _ptr)
{
    GASNET_Safe(gasnet_AMRequestMedium0(_node,hidx_dstmfree_medhandler,(void*)(&_ptr), sizeof(void*)));
}

void glue::dstm_mallocFree(uintptr_t _ptr)
{
    MEMORY_MANAGER->free(_ptr);
}

void glue::dstm_mallocMallocRequest(uint32_t _node, size_t size, uintptr_t _ptr, uintptr_t _mutex)
{
    GASNET_Safe(gasnet_AMRequestShort5(_node,hidx_dstmmalloc_shrthandler, size,
                                       GASNETI_HIWORD(_ptr), GASNETI_LOWORD(_ptr),
                                       GASNETI_HIWORD(_mutex), GASNETI_LOWORD(_mutex)));

    GASNET_BLOCKUNTIL((*(uintptr_t*)_mutex) == 1);
}

uintptr_t glue::dstm_mallocMalloc(size_t size)
{
    return MEMORY_MANAGER->malloc(size);
}

void glue::sendNodeStats(node_stats_t node_stat)
{
    GASNET_Safe(gasnet_AMRequestMedium0(0, hidx_dstmstats_medhandler, (void*)(&node_stat), sizeof(node_stats_t)));
}

void glue::glue_gasnet_memset(uint32_t node, void *dest, int val, size_t nbytes)
{
    gasnet_memset(node, dest, val, nbytes);
}

void glue::glue_gasnet_get(void *dest, uint32_t node, void *src, size_t nbytes)
{
    gasnet_get(dest, node, src, nbytes);
}

void glue::glue_gasnet_put(uint32_t node, void *dest, void *src, size_t nbytes)
{
    gasnet_put(node, dest, src, nbytes);
}
