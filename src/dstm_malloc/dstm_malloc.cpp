/*
 * dstm_malloc.cpp
 *
 *  Created on: 14.05.2011
 *      Author: Igor Danilov
 */

#include "dstm_malloc.h"
#include "stdio.h"
#include "stdint.h"

#include "gasnet.h"
#include "gasnet_tools.h"

#include "../glue.h"
#include "../profiling.h"
#include "../dstm_lib_p1/dstm_timestamps.h"

#include "buddy.h"

gasnett_mutex_t G_MALLOC_MTX = GASNETT_MUTEX_INITIALIZER;

uintptr_t LOW_BOUND;

struct buddy *G_BUDDY_S;

int ALL_UNITS = 0;

void dstm_malloc_init(uintptr_t _lb)
{
#ifdef LOGGING
    VLOG(2)<<"Inside function "<<__PRETTY_FUNCTION__<<" LOW_BOUND = "<< std::hex <<"0x"<<_lb;
#endif
    LOW_BOUND = _lb;

//	G_BUDDY_S = buddy_new(BUDDY_LEVEL);
}

void* dstm_malloc(size_t __size, uint32_t __node)
{
#ifdef LOGGING
    VLOG(2)<<"Inside function "<<__PRETTY_FUNCTION__<<" size = "<<__size<<" node = "<<__node;
#endif
    void *mem_ptr;

    if (__node == UINT_MAX) {
		int offset;

		int units = __size >> MAU_SIZE;

		if ((units << MAU_SIZE) < __size)
			units = (__size >> MAU_SIZE) + 1;

		gasnett_mutex_lock(&G_MALLOC_MTX);

//		offset = buddy_alloc(G_BUDDY_S, units);
        offset = ALL_UNITS;
        ALL_UNITS += units;

		gasnett_mutex_unlock(&G_MALLOC_MTX);

		if (offset == -1)
			return NULL;

		mem_ptr = (void*) (LOW_BOUND + offset * (1 << MAU_SIZE));

        mem_ptr = (void*) ((uintptr_t) mem_ptr | TO_NODE_TS(gasnet_mynode()));
#ifdef LOGGING
    VLOG(2)<<"Out function "<<__PRETTY_FUNCTION__<<" ptr = "<<mem_ptr<<" units = "<<units;
#endif
    } else {
        uintptr_t mllcMtx = 0;
		//gasnett_mutex_init(&mllcMtx);
        glue::dstm_mallocMallocRequest(__node, __size, (uintptr_t)&mem_ptr, (uintptr_t)&mllcMtx);
		//gasnett_mutex_lock(&mllcMtx);
		//gasnett_mutex_destroy(&mllcMtx);
	}
    return mem_ptr;
}

void dstm_free(void *__ptr)
{
    void *ptr = (void*)(ADDR_FROM_ADDRESS(__ptr ));

//	fprintf(stderr,"%s %p node %d - 1\n",__PRETTY_FUNCTION__,__ptr,gasnet_mynode());
#ifdef LOGGING
    VLOG(2)<<"Inside function "<<__PRETTY_FUNCTION__<<" ptr = "<<__ptr<<" node = "<< gasnet_mynode();
#endif

    gasnet_node_t node = NODE_FROM_ADDRESS(__ptr);

	int size = 0;

	if (node == gasnet_mynode()) {
		gasnett_mutex_lock(&G_MALLOC_MTX);
//		size = buddy_size(G_BUDDY_S,((uintptr_t) ptr - LOW_BOUND) >> MAU_SIZE);
//		buddy_free(G_BUDDY_S, ((uintptr_t) ptr - LOW_BOUND) >> MAU_SIZE);
//		fprintf(stderr,"%s %p size %d node %d - 2\n",__PRETTY_FUNCTION__,__ptr,size,gasnet_mynode());
        size = 1;
        glue::glue_gasnet_memset(node, ptr, 0, size*(1 << MAU_SIZE));
		gasnett_mutex_unlock(&G_MALLOC_MTX);
	}else
		glue::dstm_mallocFreeRequest(node,(uintptr_t)__ptr);

//	fprintf(stderr,"%s %p size %d node %d\n",__PRETTY_FUNCTION__,__ptr,size,gasnet_mynode());
}

void dstm_malloc_shutdown()
{
//	buddy_delete(G_BUDDY_S);
}

void *dstm_memcpy(void* dst, void* src, size_t n)
{
    gasnet_node_t src_node = NODE_FROM_ADDRESS(src);

    glue::glue_gasnet_get(dst, src_node, src, n);

	return dst;
}
