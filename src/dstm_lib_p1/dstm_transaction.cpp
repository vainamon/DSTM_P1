/*
 * dstm_transaction.cpp
 *
 *  Created on: 10.02.2012
 *      Author: igor
 */

#include "dstm_transaction.h"

#include "dstm_transaction_impl.h"

#include "dstm_locks_cache.h"

#include "../profiling.h"

DSTMTransaction::DSTMTransaction() : status(TX_IDLE), maxRequestedLocksPerNode (10), rlNodes(0)
{
    gasnett_mutex_init(&rlMutex);
    gasnett_cond_init(&rlCond);
#ifdef TRANSACTIONS_STATS
    stats.total_aborts = 0;
    stats.rsval_aborts = 0;
    stats.grablocks_aborts = 0;
    stats.wr_confl_aborts = 0;
    stats.ww_confl_aborts = 0;
    stats.invmem_r_aborts = 0;
    stats.invmem_w_aborts = 0;
    stats.unknwn_aborts = 0;
    stats.explicit_aborts = 0;
#endif

    VC.resize(gasnet_nodes());
}

DSTMTransaction::~DSTMTransaction()
{
    gasnett_mutex_destroy(&rlMutex);
    gasnett_cond_destroy(&rlCond);
}

bool DSTMTransaction::begin()
{
    status = TX_ACTIVE;

    rlChangedObjects.clear();

    std::fill(VC.begin(), VC.end(), 0);

#ifdef LOGGING
    VLOG(3)<<"TX begin "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif

    return true;
}

bool DSTMTransaction::getLocks()
{
    write_set_t::const_iterator it;

    boost::unordered_map<uint32_t, uint64_t*> objects_versions_array;

    it = write_set.begin();

    while (it != write_set.end()) {

        do {
            uint32_t node = NODE_FROM_ADDRESS(it->first);

            if (objects_versions_array.find(node) == objects_versions_array.end()) {

                uint64_t *objvers = (uint64_t*) malloc((2 * maxRequestedLocksPerNode + 2) * sizeof(uint64_t));
                memset((void*)objvers, 0, (2 * maxRequestedLocksPerNode + 2) * sizeof(uint64_t));

                objects_versions_array[node] = objvers;

                objvers[0] = (uintptr_t)this;		/// pointer to the transaction descriptor
                objvers[1] = 0;						/// objects count
            }

            uint64_t *objvers = objects_versions_array[node];

            uint64_t objects_count = objvers[1];

            /// object address
            objvers[objects_count + 2] = ADDR_FROM_ADDRESS(it->first);

            /// object version
#ifdef VARIANT1
            objvers[objects_count + maxRequestedLocksPerNode + 2] = *((uint64_t*)it->second);
#endif
#ifdef VARIANT2
            objvers[objects_count + maxRequestedLocksPerNode + 2] = it->second;
#endif
            it++;

            // TODO: node count limitation

            /// object count
            if (++objvers[1] == maxRequestedLocksPerNode)
                break;

        } while (it != write_set.end());

        rlNodes = 0;

        boost::unordered_map<uint32_t, uint64_t*>::const_iterator rl_it;

        for(rl_it = objects_versions_array.begin(); rl_it != objects_versions_array.end(); rl_it++)
        {
            uint32_t node = rl_it->first;
            uint64_t *objvers = rl_it->second;

            glue::requestLocks(objvers, (2 * maxRequestedLocksPerNode + 2) * sizeof(uint64_t), node);
        }

        /// wait request processing
        //...

        gasnett_mutex_lock(&rlMutex);

        while(rlNodes < objects_versions_array.size())
            gasnett_cond_wait(&rlCond,&rlMutex);

        gasnett_mutex_unlock(&rlMutex);

#ifdef LOGGING
        VLOG(3)<<"TX getLocks "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif

        for(rl_it = objects_versions_array.begin(); rl_it != objects_versions_array.end(); rl_it++)
            free((uint64_t*)rl_it->second);

        objects_versions_array.clear();

        if(!rlChangedObjects.empty()){
            releaseLocks(it);
#ifdef CACHE
            for(int i = 0; i< rlChangedObjects.size(); i++)
                globalCache->addVersionToCache(rlChangedObjects[i], *((uint64_t*)write_set[rlChangedObjects[i]]));
#endif

            rlChangedObjects.clear();
#ifdef CACHE
            /*write_set_t::const_iterator it_invalidate = it;

            for(it_invalidate = it; it_invalidate != write_set.end(); it_invalidate++)
                globalCache->invalidate(it_invalidate->first,*((uint64_t*)it_invalidate->second));*/
            /*read_set_t::const_iterator it_invalidate;
            it_invalidate = read_set.begin();
            while (it_invalidate != read_set.end()) {

                globalCache->invalidate(it_invalidate->first, it_invalidate->second);

                it_invalidate++;
            }*/
#endif
            return false;
        }
    }

    return true;
}

void DSTMTransaction::releaseLocks(write_set_t::const_iterator it_end)
{
    write_set_t::const_iterator it;

    gasnet_node_t node;

    uintptr_t lock_addr;

    for (it = write_set.begin(); it != it_end; it++) {
        /// if current object from write set not in changed objects we must release early grabbed lock
        if (std::find(rlChangedObjects.begin(), rlChangedObjects.end(), it->first) == rlChangedObjects.end())
        {
            node = NODE_FROM_ADDRESS(it->first);

            lock_addr = globalMemMgr->getObjectLockAddr(it->first);
#ifdef VARIANT1
            glue::glue_gasnet_put(node, (void*) ADDR_FROM_ADDRESS(lock_addr), (void*) (it->second), sizeof(uint64_t));
#endif
#ifdef VARIANT2
            glue::glue_gasnet_put(node, (void*) ADDR_FROM_ADDRESS(lock_addr), (void*) (&it->second), sizeof(uint64_t));
#endif
        }
    }

    rlChangedObjects.clear();
}

void DSTMTransaction::locksRequestProcessed(uint64_t* objs, uint32_t nobjs)
{
    gasnett_mutex_lock(&rlMutex);

    if(nobjs > 0){
        for(int i = 0; i<nobjs; i++)
            rlChangedObjects.push_back(objs[i]);
    }

    rlNodes++;

    gasnett_cond_signal(&rlCond);

    gasnett_mutex_unlock(&rlMutex);
}

bool DSTMTransaction::validate(uint64_t _ts)
{
    uint64_t ts, ts_cached, ts_cur;

    uint64_t tsn_new = VERSION_TS(_ts);
    uint32_t node = NODE_TS(_ts);

    uint64_t tsn_old = VC[node];

    read_set_t::const_iterator rs_it;
#ifdef VARIANT1
    write_set_t::const_iterator ws_it;
#endif

#ifdef VARIANT3
    DSTMLocksCache *locksCache = new DSTMLocksCache;

    locksCache->setCacheLine(4);

    uintptr_t lock_addr;
#endif
    if (tsn_new > tsn_old) {
        for (rs_it = read_set.begin(); rs_it != read_set.end(); rs_it++)
        {
#ifdef VARIANT2
            ts = rs_it->second;
#endif
#ifdef VARIANT1
            ts = *(uint64_t*) rs_it->second;
#endif
            if (isTimestampGreater(_ts, ts)) {
#ifdef CACHE
                ts_cached = globalCache->getCachedVersion(rs_it->first);

                if ((LOCKBIT_TS(ts_cached)) || (isTimestampGreater(ts_cached, ts)))
                    return false;
#endif
#ifdef VARIANT3
                lock_addr = globalMemMgr->getObjectLockAddr(rs_it->first);

                ts_cur = locksCache->get(lock_addr);
#else
                ts_cur = globalCache->getVersion(globalMemMgr->getObjectLockAddr(rs_it->first));
#endif

                if ((LOCKBIT_TS(ts_cur)) || (isTimestampGreater(ts_cur, ts)))
                {
#ifdef CACHE
                    globalCache->addVersionToCache(rs_it->first, ts_cur);
#endif
                    return false;
                }
            }
        }

#ifdef VARIANT1
        for (ws_it = write_set.begin(); ws_it != write_set.end(); ws_it++)
        {
            ts = *(uint64_t*) ws_it->second;
            if (isTimestampGreater(_ts, ts)) {
#ifdef CACHE
                ts_cached = globalCache->getCachedVersion(ws_it->first);

                if ((LOCKBIT_TS(ts_cached)) || (isTimestampGreater(ts_cached, ts)))
                    return false;
#endif
#ifdef VARIANT3
                lock_addr = globalMemMgr->getObjectLockAddr(ws_it->first);

                ts_cur = locksCache->get(lock_addr);
#else
                ts_cur = globalCache->getVersion(globalMemMgr->getObjectLockAddr(ws_it->first));
#endif

                if ((LOCKBIT_TS(ts_cur)) || (isTimestampGreater(ts_cur, ts)))
                {
#ifdef CACHE
                    globalCache->addVersionToCache(ws_it->first, ts_cur);
#endif
                    return false;
                }
            }
        }
#endif
        VC[node] = tsn_new;
    }

    return true;
}

bool DSTMTransaction::validateAll()
{
    read_set_t::const_iterator it;

    uint64_t ts, ts_cached, ts_cur;

    it = read_set.begin();

#ifdef VARIANT3
    DSTMLocksCache *locksCache = new DSTMLocksCache;

    locksCache->setCacheLine(4);

    uintptr_t lock_addr;
#endif

    for(it = read_set.begin(); it != read_set.end(); it++)
    {
#ifdef VARIANT2
        if(write_set.find(it->first) != write_set.end())
            continue;
        ts = it->second;
#endif
#ifdef VARIANT1
        ts = *(uint64_t*) it->second;
#endif
#ifdef CACHE
        ts_cached = globalCache->getCachedVersion(it->first);

        if ((LOCKBIT_TS(ts_cached)) || (isTimestampGreater(ts_cached, ts))){
            return false;
        }
#endif
#ifdef VARIANT3
        lock_addr = globalMemMgr->getObjectLockAddr(it->first);

        ts_cur = locksCache->get(lock_addr);
#else
        ts_cur = globalCache->getVersion(globalMemMgr->getObjectLockAddr(it->first));
//	fprintf(stderr, "0x%llx 0x%llx\n", ts_cur, ts);
#endif
        if ((LOCKBIT_TS(ts_cur)) || (isTimestampGreater(ts_cur, ts)))
        {
#ifdef CACHE
            globalCache->addVersionToCache(it->first, ts_cur);
#endif
            return false;
        }
    }

    return true;
}

//void DSTMTransaction::invalidateRSValuesInCache(){
//    read_set_t::const_iterator it;

//    it = read_set.begin();

//    while (it != read_set.end()) {

//        globalCache->invalidate(it->first, it->second);

//        it++;
//    }
//}

//bool DSTMTransaction::validateReadSet()
//{
//    read_set_t::const_iterator it;

//    uintptr_t lock_addr;

//    it = read_set.begin();

//    bool result = true;

//    uint64_t version;

//    DSTMLocksCache *locksCache = new DSTMLocksCache;

//    locksCache->setCacheLine(3);

//    //	fprintf(stderr,"%s size %d node %d\n",__PRETTY_FUNCTION__,read_set.size(),gasnet_mynode());
//    while (it != read_set.end()) {
//        //if (globalCache->getVersion(it->first) > it->second)
//        //	result = false;
//        //else {

//        lock_addr = globalMemMgr->getObjectLockAddr(it->first);

//        version = locksCache->get(lock_addr);

//        if (version > it->second) {
//#ifdef CACHE
//            globalCache->invalidate(it->first, version);
//#endif
//            //read_set.erase(it->first);
//            result = false;
//            break;
//        }

//        //}

//        it++;
//    }

//    delete locksCache;
//#ifdef CACHE
//    /*if (result == false) {
//        invalidateRSValuesInCache();
//    }*/
//#endif
//    return result;
//}

void DSTMTransaction::commitWriteSetValues()
{
#ifdef LOGGING
    VLOG(3)<<"TX commitWriteSetValues "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif
#ifdef VARIANT1
    write_set_t::const_iterator it;
#endif
#ifdef VARIANT2
    write_set_values_t::const_iterator it;
#endif
    gasnet_node_t node;

    //gasnet_handle_t handle;

    //gasnet_begin_nbi_accessregion();

#ifdef VARIANT1
    for (it = write_set.begin(); it != write_set.end(); it++) {

        node = NODE_FROM_ADDRESS(it->first);

        glue::glue_gasnet_put(node, (void*) ADDR_FROM_ADDRESS(it->first),
                         (void*) (it->second + sizeof(uint64_t)), 1 << CDU_SIZE);
    }
#endif

#ifdef VARIANT2
    for (it = write_set_values.begin(); it != write_set_values.end(); it++) {

        node = NODE_FROM_ADDRESS(it->first);

        glue::glue_gasnet_put(node, (void*) ADDR_FROM_ADDRESS(it->first),
                         (void*) (it->second + sizeof(uint64_t)), *(uint64_t*) (it->second));
    }
#endif

    //handle = gasnet_end_nbi_accessregion();

    //gasnet_wait_syncnb(handle);
}

void DSTMTransaction::releaseLocksWithNewClock()
{
    write_set_t::const_iterator it;

    gasnet_node_t node;

    uintptr_t lock_addr;

    uint64_t new_clock = glue::getNextClock();
    uint64_t new_version = TO_NODE_TS(gasnet_mynode()) | new_clock;

    //gasnet_handle_t handle;

    //gasnet_begin_nbi_accessregion();

    for (it = write_set.begin(); it != write_set.end(); it++) {

        node = NODE_FROM_ADDRESS(it->first);

        lock_addr = globalMemMgr->getObjectLockAddr(it->first);

        glue::glue_gasnet_put(node, (void*) ADDR_FROM_ADDRESS(lock_addr),
                         (void*) (&new_version),  sizeof(uint64_t));
#ifdef CACHE
        //        globalCache->invalidate(it->first,new_clock);
        globalCache->addVersionToCache(it->first, new_version);
#endif
    }
    //handle = gasnet_end_nbi_accessregion();

    //gasnet_wait_syncnb(handle);

#ifdef CACHE
    /*for (it = write_set.begin(); it != write_set.end(); it++) {
        globalCache->invalidate(it->first,new_clock);
    }*/
#endif

#ifdef LOGGING
    VLOG(3)<<"NEW CLOCK = "<<new_clock<<" TX "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif
}

//bool DSTMTransaction::commit()
//{
//#ifdef LOGGING
//    VLOG(3)<<"TX committing "<<this<<" node "<<gasnet_mynode()<<"\n";
//#endif

//    /// validate read-set
//    if (!validateReadSet()){
//#ifdef LOGGING
//        VLOG(3)<<"Read-set validation failed! TX "<<this<<" node "<<gasnet_mynode()<<"\n";
//#endif

//        abort(ABORT_READSET_VAL);
//    }

//    if (!write_set.empty()){
//        /// get all locks in write-set with object version validation check
//        if (!getLocks()) {

//#ifdef LOGGING
//            VLOG(3)<<"Locks grabbing failed! TX "<<this<<" node "<<gasnet_mynode()<<"\n";
//#endif

//            abort(ABORT_GRABBING_LOCKS);
//        }

//        /// commit all write-set objects with new clock value and release locks
//        commitWriteSetValues();

//        releaseLocksWithNewClock();

//        clearWriteSet();
//    }

//    clearReadSet();

//    status = TX_COMMITTED;

//#ifdef LOGGING
//    VLOG(3)<<"TX commit "<<this<<" node "<<gasnet_mynode()<<"\n";
//#endif

//    return true;
//}

bool DSTMTransaction::commit()
{
//fprintf(stderr, "in commit\n");
#ifdef LOGGING
    VLOG(3)<<"TX committing "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif

    if (!write_set.empty()) {
        /// get all locks in write-set with object version validation check
        if (!getLocks()) {
//fprintf(stderr, "get_locks\n");
#ifdef LOGGING
            VLOG(3)<<"Locks grabbing failed! TX "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif
            abort(ABORT_GRABBING_LOCKS);
        }
    }
    
    /// validate read-set
    if (!validateAll()){
//fprintf(stderr, "val_all\n");
#ifdef LOGGING
        VLOG(3)<<"Read-set validation failed! TX "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif
        if (!write_set.empty())
            releaseLocks(write_set.end());
        abort(ABORT_READSET_VAL);
    }

    /// commit all write-set objects with new clock value and release locks
    commitWriteSetValues();

    releaseLocksWithNewClock();

    clearWriteSet();

    clearReadSet();

    status = TX_COMMITTED;

#ifdef LOGGING
    VLOG(3)<<"TX commit "<<this<<" node "<<gasnet_mynode()<<"\n";
#endif
//fprintf(stderr, "out commit\n");
    return true;
}

void DSTMTransaction::rollback()
{
    abort(ABORT_EXPLICIT);
}

void DSTMTransaction::saveCheckpoint()
{
    if(sigsetjmp(env,1) != 0){
    }else{
    }
}

void DSTMTransaction::abort(uint64_t reason)
{
#ifdef LOGGING
    VLOG(3)<<"TX abort "<<this<<" reason "<<reason<<" node "<<gasnet_mynode()<<"\n";
#endif

#ifdef TRANSACTIONS_STATS
    stats.total_aborts++;

    switch (reason) {
    case ABORT_READSET_VAL:
        stats.rsval_aborts++;
        break;
    case ABORT_READSET_VAL | ABORT_WR_CONFLICT:
        stats.rsval_aborts++;
        //stats.wr_confl_aborts++;
        break;
    case ABORT_READSET_VAL | ABORT_WW_CONFLICT:
        stats.rsval_aborts++;
        //stats.ww_confl_aborts++;
        break;
    case ABORT_WR_CONFLICT:
        stats.wr_confl_aborts++;
        break;
    case ABORT_WW_CONFLICT:
        stats.ww_confl_aborts++;
        break;
    case ABORT_INVALID_MEMORY_R:
        stats.invmem_r_aborts++;
        break;
    case ABORT_INVALID_MEMORY_W:
        stats.invmem_w_aborts++;
        break;
    case ABORT_GRABBING_LOCKS:
        stats.grablocks_aborts++;
        break;
    case ABORT_EXPLICIT:
        stats.explicit_aborts++;
        break;
    default:
        stats.unknwn_aborts++;
        break;
    };
#endif

    clearWriteSet();
    clearReadSet();

    status = TX_ABORTED;

    siglongjmp(env,-1);
}

void DSTMTransaction::clearWriteSet()
{
#ifdef VARIANT1
    write_set_t::const_iterator it;

    for (it = write_set.begin(); it!=write_set.end(); it++)
        free((void*)it->second);
#endif

    write_set.clear();

#ifdef VARIANT2
    write_set_values_t::const_iterator it;

    for (it = write_set_values.begin(); it!=write_set_values.end(); it++)
        free((void*)it->second);

    write_set_values.clear();
#endif
}

void DSTMTransaction::clearReadSet()
{
#ifdef VARIANT1
    read_set_t::const_iterator it;

    for (it = read_set.begin(); it!=read_set.end(); it++)
        free((void*)it->second);
#endif
    read_set.clear();
}

template inline uint32_t DSTMTransaction::read<uint32_t>(uint32_t*);
template inline uint64_t DSTMTransaction::read<uint64_t>(uint64_t*);

template void inline DSTMTransaction::write<uint32_t>(uint32_t*,uint32_t);
template void inline DSTMTransaction::write<uint64_t>(uint64_t*,uint64_t);
