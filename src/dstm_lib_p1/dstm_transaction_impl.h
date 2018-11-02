/*
 * dstm_transaction_impl.h
 *
 *  Created on: 15.02.2012
 *      Author: igor
 */

#ifndef DSTM_TRANSACTION_IMPL_H_
#define DSTM_TRANSACTION_IMPL_H_
//#define VARIANT2
#include "dstm_transaction.h"
#include "dstm_timestamps.h"
#include "../glue.h"
#include "../profiling.h"

//template<class T>
//T DSTMTransaction::read(uintptr_t addr)
//{
//	reads++;
//	if (addr == 0){
//		abort(ABORT_INVALID_MEMORY_R);
//	}

//	uintptr_t block_lb = (((uintptr_t)addr) >> CDU_SIZE) << CDU_SIZE;

//	uint64_t version = 0;

//	T value;

//	if(write_set.find(block_lb) != write_set.end()){
//#ifdef CACHE
//		//globalCache->get<T>(addr,globalMemMgr->getObjectLockAddr(addr),&version);
//		version = globalCache->getVersion(block_lb);

//		if((!(version >> 63))&&(glue::increaseClock(version)))
//			fprintf(stderr,"gav-gav-4!!\n");

//		if (read_set[block_lb] < version){
//			fprintf(stderr,"gav-gav-44!!\n");
//			//invalidateRSValuesInCache();
//			abort(ABORT_WR_CONFLICT);
//		}

//		if (brv < version) {
//			fprintf(stderr,"gav-gav-3!!\n");
//			brv = version;//glue::getCurrentClock();
//			if (!validateReadSet())
//				abort(ABORT_WR_CONFLICT | ABORT_READSET_VAL);
//		}
//#endif
//		value = *((T*)(write_set[block_lb] + sizeof(uint64_t) + ((addr - block_lb) & ADDRESS_MASK)));
//		return value;
//	}

//    value = globalCache->get<T>(addr, globalMemMgr->getObjectLockAddr(addr), &version);

//	if (read_set.find(block_lb) == read_set.end()){

////		value = globalCache->get<T>(addr,globalMemMgr->getObjectLockAddr(addr),&version);
///// TODO spin awhile, then abort. Insert exponential sleep function.
////		int count = 0;
////		int ncount = 0;
////		struct timespec req = {0};
////		req.tv_sec = 0;
////		int mynode = (((NODE_MASK << 48) & (addr)) >> 48 == gasnet_mynode());

////		if (((NODE_MASK << 48) & (addr)) >> 48 == gasnet_mynode()){
////			req.tv_nsec = 50;//50;
////			ncount = 100;
////		}else{
////			req.tv_nsec = 1000;//1000
////			ncount = 7;
////		}
//		if(version >> 63)
//			abort(version);

////		while(version >> 63){
////			if (count > 0)
////				abort(version);
////			//nanosleep(&req,(struct timespec*)NULL);
////			value = globalCache->get<T>(addr,globalMemMgr->getObjectLockAddr(addr),&version);
////			count++;
////		}
//		read_set[block_lb] = version;
////		rs.push_back(block_lb);

//		if(glue::increaseClock(version));

//	} else {
////		value = globalCache->get<T>(addr,globalMemMgr->getObjectLockAddr(addr),&version);

//		if((!(version >> 63))&&(glue::increaseClock(version)));

//		if (read_set[block_lb] < version){
//			//fprintf(stderr,"gav-gav!!\n");
//			//invalidateRSValuesInCache();

//			abort(ABORT_WR_CONFLICT);
//		}

//	}

//	if (brv < version) {
//		if (!validateReadSet())
//			abort(ABORT_WR_CONFLICT | ABORT_READSET_VAL);
//		brv = version;
//	}

////	fprintf(stderr,"%s %p ret 0x%llx val 0x%llx\n",__PRETTY_FUNCTION__,addr,value,*(uint64_t*)addr);
//	return value;
//}

//template<class T>
//void DSTMTransaction::write(T *addr, T value)
//{
//	if (addr == NULL)
//		abort(ABORT_INVALID_MEMORY_W);

////	fprintf(stderr,"%s %p val 0x%llx node %d\n",__PRETTY_FUNCTION__,addr,value,gasnet_mynode());
//	uintptr_t block_lb = (((uintptr_t)addr) >> CDU_SIZE) << CDU_SIZE;

//	uint64_t version = 0;

//	if(write_set.find(block_lb) == write_set.end()){

//		uintptr_t write_ent = (uintptr_t)malloc((1 << CDU_SIZE)+sizeof(uint64_t));

//		write_set[block_lb] = write_ent;

//		//globalCache->get<T>((uintptr_t)addr,globalMemMgr->getObjectLockAddr((uintptr_t)addr),&version,write_ent);

//		version = globalCache->getVersion(globalMemMgr->getObjectLockAddr((uintptr_t)addr));
//		*((uint64_t*)write_ent) = version;

//		if (read_set.find(block_lb) == read_set.end()){
//			//if (globalCache->inCache((uintptr_t)addr)){
//	/// TODO spin awhile, then abort. Insert exponential sleep function.
////				int count = 0;
////				struct timespec req = {0};
////				req.tv_sec = 0;
////				req.tv_nsec = 100;
////				//int mynode = (((NODE_MASK << 48) & ((uintptr_t)addr)) >> 48 == gasnet_mynode());
////				while(version >> 63){
////					if (count > 0)
////						abort(ABORT_EXPLICIT);
////					//nanosleep(&req,(struct timespec*)NULL);
////					globalCache->get<T>((uintptr_t)addr,globalMemMgr->getObjectLockAddr((uintptr_t)addr),&version,write_ent);
////					count++;
////				}
//				if(version >> 63)
//					abort(ABORT_EXPLICIT);

////				read_set[block_lb] = version;
////				rs.push_back(block_lb);

//				if(glue::increaseClock(version));

//			//}else{
//			//	*((uint64_t*)write_ent) = glue::getCurrentClock();
//			//}
//		} else {

//			if((!(version >> 63))&&(glue::increaseClock(version)));

//			if (read_set[block_lb] < version){
//				//fprintf(stderr,"myav-myav!!\n");
//				//invalidateRSValuesInCache();

//				abort(ABORT_WW_CONFLICT);
//			}
//			//read_set.erase(block_lb);
//		}

////		if (brv < version){
////			brv = version;//glue::getCurrentClock();
////			if(!validateReadSet())
////				abort(ABORT_WW_CONFLICT | ABORT_READSET_VAL);
////		}

//	}
//    *((T*)(write_set[block_lb] + sizeof(uint64_t))) = 100500;
//    *((T*)(write_set[block_lb] + 2*sizeof(uint64_t))) = 100500;
//    *((T*)(write_set[block_lb] + sizeof(uint64_t) + (((uintptr_t)addr - block_lb) & ADDRESS_MASK))) = value;
//}

#ifdef VARIANT1
template<class T>
T DSTMTransaction::read(T *addr)
{
#ifdef LOGGING
    VLOG(3)<<"TX "<<this<<" read: node "<<gasnet_mynode()<<" addr "<<(void*)addr<<"\n";
#endif
//printf("READ 0x%llx\n",addr);
//fflush(stdout);
    if (addr == 0)
        abort(ABORT_INVALID_MEMORY_R);

    uintptr_t block_lb = (((uintptr_t)addr) >> CDU_SIZE) << CDU_SIZE;

    uint64_t timestamp = 0;

    T value;

    if(write_set.find(block_lb) != write_set.end())
    {
        value = *((T*)(write_set[block_lb] + sizeof(uint64_t) + (ADDR_FROM_ADDRESS((uintptr_t)addr - block_lb))));
        return value;
    }

    if(read_set.find(block_lb) != read_set.end())
    {
        value = *((T*)(read_set[block_lb] + sizeof(uint64_t) + (ADDR_FROM_ADDRESS((uintptr_t)addr - block_lb))));
        return value;
    }

    uintptr_t read_ent = (uintptr_t)malloc((1 << CDU_SIZE)+sizeof(uint64_t));

    read_set[block_lb] = read_ent;

    value = globalCache->readCell<T>((uintptr_t)addr, globalMemMgr->getObjectLockAddr((uintptr_t)addr), &timestamp, read_ent);
#ifdef CACHE
    globalCache->addVersionToCache(block_lb, timestamp);
#endif
    if(LOCKBIT_TS(timestamp)) {
        abort(timestamp);
    }

    glue::increaseClock(VERSION_TS(timestamp));

    if (!validate(timestamp))
        abort(ABORT_WR_CONFLICT | ABORT_READSET_VAL);

    return value;
}

template<class T>
void DSTMTransaction::write(T *addr, T value)
{
#ifdef LOGGING
    VLOG(3)<<"TX "<<this<<" write: node "<<gasnet_mynode()<<" addr "<<addr<<" value "<<value<<"\n";
#endif
//printf("WRITE 0x%llx %d\n",addr,value);
//fflush(stdout);

    if (addr == NULL)
        abort(ABORT_INVALID_MEMORY_W);

    uintptr_t block_lb = (((uintptr_t)addr) >> CDU_SIZE) << CDU_SIZE;

    uint64_t timestamp = 0;

    if(write_set.find(block_lb) == write_set.end()){

        uintptr_t write_ent = (uintptr_t)malloc((1 << CDU_SIZE)+sizeof(uint64_t));

        write_set[block_lb] = write_ent;

        if (read_set.find(block_lb) == read_set.end()){
            globalCache->readCell<T>((uintptr_t)addr, globalMemMgr->getObjectLockAddr((uintptr_t)addr), &timestamp, write_ent);
#ifdef CACHE
            globalCache->addVersionToCache(block_lb, timestamp);
#endif
            if(LOCKBIT_TS(timestamp))
                abort(timestamp);

            glue::increaseClock(VERSION_TS(timestamp));

            if (!validate(timestamp))
                abort(ABORT_WR_CONFLICT | ABORT_READSET_VAL);
        } else {
            memcpy((void*) write_ent, (void*) read_set[block_lb], (1 << CDU_SIZE)+sizeof(uint64_t));
            read_set.erase(block_lb);
        }
    }

    *((T*)(write_set[block_lb] + sizeof(uint64_t) + (ADDR_FROM_ADDRESS((uintptr_t)addr - block_lb)))) = value;
}
#endif

#ifdef VARIANT2
template<class T>
T DSTMTransaction::read(T *addr)
{
#ifdef LOGGING
    VLOG(3)<<"TX "<<this<<" read: node "<<gasnet_mynode()<<" addr "<<(void*)addr<<"\n";
#endif
//fprintf(stderr, "READ: 0x%llx\n", addr);
    if (addr == 0)
        abort(ABORT_INVALID_MEMORY_R);

    uintptr_t block_lb = (((uintptr_t)addr) >> CDU_SIZE) << CDU_SIZE;

    uint64_t timestamp = 0;

    T value;

    if(write_set_values.find(uintptr_t(addr)) != write_set.end())
    {
        value = *((T*)(write_set_values[uintptr_t(addr)] + sizeof(uint64_t)));
        return value;
    }

    value = globalCache->readCell<T>((uintptr_t)addr, globalMemMgr->getObjectLockAddr((uintptr_t)addr), &timestamp, NULL);
#ifdef CACHE
    globalCache->addVersionToCache(block_lb, timestamp);
#endif
    if(LOCKBIT_TS(timestamp)) {
        abort(timestamp);
    }

    glue::increaseClock(VERSION_TS(timestamp));

    if(write_set.find(block_lb) != write_set.end())
    {
        if (!isTimestampsEquivalent(write_set[block_lb], timestamp))
            abort(ABORT_WR_CONFLICT);
    }

    if (!validate(timestamp))
        abort(ABORT_RW_CONFLICT | ABORT_READSET_VAL);

    read_set[block_lb] = timestamp;

    return value;
}

template<class T>
void DSTMTransaction::write(T *addr, T value)
{
#ifdef LOGGING
    VLOG(3)<<"TX "<<this<<" write: node "<<gasnet_mynode()<<" addr "<<addr<<" value "<<value<<"\n";
#endif
//fprintf(stderr, "WRITE: 0x%llx %d\n", addr,value);
    if (addr == NULL)
        abort(ABORT_INVALID_MEMORY_W);

    uintptr_t block_lb = (((uintptr_t)addr) >> CDU_SIZE) << CDU_SIZE;

    uint64_t timestamp = 0;

    if(write_set_values.find(uintptr_t(addr)) == write_set.end()){

        uintptr_t write_ent = (uintptr_t)malloc(sizeof(uint64_t) + sizeof(uint64_t));

        write_set_values[uintptr_t(addr)] = write_ent;

        if (read_set.find(block_lb) == read_set.end()){
            globalCache->readCell<T>((uintptr_t)addr, globalMemMgr->getObjectLockAddr((uintptr_t)addr), &timestamp, NULL);
#ifdef CACHE
            globalCache->addVersionToCache(block_lb, timestamp);
#endif
            if(LOCKBIT_TS(timestamp))
                abort(timestamp);

            glue::increaseClock(VERSION_TS(timestamp));

            if(write_set.find(block_lb) != write_set.end())
            {
                if (!isTimestampsEquivalent(write_set[block_lb], timestamp))
                    abort(ABORT_WR_CONFLICT);
            }

            if (!validate(timestamp))
                abort(ABORT_WR_CONFLICT | ABORT_READSET_VAL);

            write_set[block_lb] = timestamp;
            *((uint64_t*)(write_set_values[uintptr_t(addr)])) = sizeof(T);
        } else {
            write_set[block_lb] = read_set[block_lb];
            *((uint64_t*)(write_set_values[uintptr_t(addr)])) = sizeof(T);
        }
    }

    *((uint64_t*)(write_set_values[uintptr_t(addr)] + sizeof(uint64_t))) = value;
}
#endif

#endif /* DSTM_TRANSACTION_IMPL_H_ */
