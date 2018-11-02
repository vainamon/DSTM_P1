/*
 * mem_mgr.h
 *
 *  Created on: 06.02.2012
 *      Author: igor
 */

#ifndef GASNET_MEM_MGR_H_
#define GASNET_MEM_MGR_H_

#include <stdint.h>
#include <vector>
#include <gasnet.h>
#include "../dstm_malloc/dstm_malloc.h"
#include "memory_manager.h"

class GASNetMemoryManager : public MemoryManager
{
public:
	GASNetMemoryManager();

	void init(gasnet_seginfo_t *segs, gasnet_node_t size);

	uintptr_t getLocksAddr(gasnet_node_t node)
	{
		return locks_addr[node];
	}

	uintptr_t getObjsAddr(gasnet_node_t node)
	{
		return objects_addr[node];
	}

	uintptr_t getObjectLockAddr(uintptr_t addr);

    uintptr_t getObjectLockAddr(uintptr_t addr, uint32_t node);

	void free(uintptr_t ptr);

	uintptr_t malloc(size_t size);

	~GASNetMemoryManager();

private:
	std::vector<uintptr_t> locks_addr;
	std::vector<uintptr_t> objects_addr;
};

#endif /* GASNET_MEM_MGR_H_ */
