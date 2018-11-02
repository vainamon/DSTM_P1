/*
 * glue.h
 *
 *  Created on: 28.02.2012
 *      Author: igor
 */

#ifndef GLUE_H_
#define GLUE_H_

#include <stdint.h>
#include <stddef.h>

#include "dstm_lib_p1/dstm_node_stats.h"

namespace glue {

	void setMemoryManager(uintptr_t);

	void requestLocks(uint64_t*, size_t, uint32_t);

	/*
	 * @return 	NULL - if all locks grabbed successfully, or pointer to array of objects which locks were not grabbed (this means abort)
	 */
	uint64_t* processLocksRequest(uint64_t*, size_t);

	/*
	 * @param 	tx 		transaction description pointer
	 * @param 	objs 	pointer to objects which locks were not grabbed
	 * @param	nobjs	number of objects which locks were not grabbed
	 */
	void locksRequestProcessed(uintptr_t tx,uint64_t* objs, uint32_t nobjs);

	uint64_t getCurrentClock();

	bool increaseClock(uint64_t);

	uint64_t getNextClock();

	void dstm_mallocFreeRequest(uint32_t,uintptr_t);

	void dstm_mallocFree(uintptr_t);

	void dstm_mallocMallocRequest(uint32_t,size_t,uintptr_t,uintptr_t);

	uintptr_t dstm_mallocMalloc(size_t size);

	void sendNodeStats(node_stats_t node_stat);

	void glue_gasnet_memset(uint32_t node, void *dest, int val, size_t nbytes);

	void glue_gasnet_get(void *dest, uint32_t node, void *src, size_t nbytes);
	
	void glue_gasnet_put(uint32_t node, void *dest, void *src, size_t nbytes);
}

#endif /* GLUE_H_ */
