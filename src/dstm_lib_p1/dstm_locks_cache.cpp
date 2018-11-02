/*
 * dstm_locks_cache.cpp
 *
 *  Created on: 20.04.2012
 *      Author: igor
 */

#include "dstm_locks_cache.h"

#include "../dstm_malloc/dstm_malloc.h"

#include <gasnet.h>
#include <gasnet_tools.h>
#include "../glue.h"

DSTMLocksCache::DSTMLocksCache() : cl_pow(12)
{
}

DSTMLocksCache::~DSTMLocksCache()
{
	cache_map_t::iterator it = i_map.begin();

	while(it != i_map.end())
	{
		free((void*)it->second);
		it++;
	}
}

uint64_t DSTMLocksCache::get(uintptr_t addr)
{
	uintptr_t al = (addr & ADDRESS_MASK) >> cl_pow;
	uintptr_t low_bound = al << cl_pow;

    gasnet_node_t node = (NODE_MASK & (addr))>>48;

	if (node == gasnet_mynode())
		return *(uint64_t*)(addr & ADDRESS_MASK);
	else {
		cache_map_t::const_iterator it_found;

		it_found = i_map.find(low_bound);

		if (it_found == i_map.end()) {
			gasnet_handle_t handle;

			uintptr_t new_cl;

			new_cl = (uintptr_t) malloc(1 << cl_pow);

            //gasnet_begin_nbi_accessregion();

            glue::glue_gasnet_get((void*) new_cl, node, (void*) low_bound, 1 << cl_pow);

            //handle = gasnet_end_nbi_accessregion();

			i_map[low_bound | ((uintptr_t) node << 48)] = new_cl;

            //gasnet_wait_syncnb(handle);

			return *(uint64_t*) ((uintptr_t) new_cl + (addr & ADDRESS_MASK)
					- low_bound);
		} else
			return *(uint64_t*) (it_found->second + ((addr & ADDRESS_MASK)
					- low_bound));
	}
}
