/*
 * gasnet_mem_mgr.cpp
 *
 *  Created on: 06.02.2012
 *      Author: igor
 */

#include "gasnet_mem_mgr.h"
#include "dstm_timestamps.h"

GASNetMemoryManager::GASNetMemoryManager()
{

}

GASNetMemoryManager::~GASNetMemoryManager()
{

}

void GASNetMemoryManager::init(gasnet_seginfo_t *segs, gasnet_node_t size)
{
	int i = 0;

	locks_addr.reserve(size);
	objects_addr.reserve(size);

	uintptr_t addr;
	uintptr_t al, ub;

    for (i = 0; i < size; i++){
		addr = (uintptr_t)segs[i].addr;
        addr = (addr >> sizeof(uint64_t)) << sizeof(uint64_t);
        locks_addr.push_back(addr);

		addr += OBJECTS_COUNT*sizeof(uint64_t);

		al = addr >> CDU_SIZE;
		ub = (al+1) << CDU_SIZE;

		objects_addr.push_back(ub);
	}

    for (i = 0; i < OBJECTS_COUNT; i++)
		*((uint64_t*)locks_addr[gasnet_mynode()]+i) = 0;
}

uintptr_t GASNetMemoryManager::getObjectLockAddr(uintptr_t addr)
{
    gasnet_node_t node = NODE_FROM_ADDRESS(addr);

    uint64_t obj_num = (ADDR_FROM_ADDRESS(addr) - objects_addr[node]) >> CDU_SIZE;

    return ((locks_addr[node] + obj_num*sizeof(uint64_t)) | TO_NODE_TS(node));
}

uintptr_t GASNetMemoryManager::getObjectLockAddr(uintptr_t addr, uint32_t node)
{
    uint64_t obj_num = (ADDR_FROM_ADDRESS(addr) - objects_addr[node]) >> CDU_SIZE;

	return (locks_addr[node]+obj_num*sizeof(uint64_t));
}

void GASNetMemoryManager::free(uintptr_t ptr)
{
	dstm_free((void*)ptr);
}

uintptr_t GASNetMemoryManager::malloc(size_t size)
{
    return (uintptr_t)dstm_malloc(size, UINT_MAX);
}
