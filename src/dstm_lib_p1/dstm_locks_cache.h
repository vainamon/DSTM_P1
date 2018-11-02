/*
 * dstm_locks_cache.h
 *
 *  Created on: 20.04.2012
 *      Author: igor
 */

#ifndef DSTM_LOCKS_CACHE_H_
#define DSTM_LOCKS_CACHE_H_

#include <stdint.h>

#include <boost/unordered_map.hpp>

#include <map>

using namespace boost;

class DSTMLocksCache {
public:
	typedef boost::unordered_map<uintptr_t,uintptr_t> cache_map_t;
//	typedef std::map<uintptr_t,uintptr_t> cache_map_t;

	DSTMLocksCache();

	uint64_t get(uintptr_t addr);

	void setCacheLine(uint32_t pow){cl_pow=pow;};

	~DSTMLocksCache();

private:
	cache_map_t i_map;
	uint32_t cl_pow;
};

#endif /* DSTM_LOCKS_CACHE_H_ */
