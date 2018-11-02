/*
 * dstm_cache.h
 *
 *  Created on: 13.01.2012
 *      Author: igor
 */

#ifndef DSTM_CACHE_H_
#define DSTM_CACHE_H_

#include <stdint.h>
#include "dstm_malloc/dstm_malloc.h"

#include "tbb/concurrent_hash_map.h"

#include <boost/functional/hash.hpp>

#include <boost/unordered_map.hpp>

#include <gasnet.h>
#include <gasnet_tools.h>

using namespace tbb;
using namespace boost;

template<typename K>
struct HashCompare {
    static size_t hash( const K& key )                  { return boost::hash_value(key); }
    static bool   equal( const K& key1, const K& key2 ) { return ( key1 == key2 ); }
};

class DSTMGASNetCache {
public:
    //typedef concurrent_hash_map<uintptr_t, uint64_t, HashCompare<uint64_t> > cache_map_t;
    typedef boost::unordered_map<uintptr_t,uintptr_t> cache_map_t;

	DSTMGASNetCache();

    template<class T>
    T get(uintptr_t addr, uintptr_t lock_addr, uint64_t* version, uintptr_t copy = NULL);

    template<class T>
    T readCell(uintptr_t addr, uintptr_t lock_addr, uint64_t* version, uintptr_t copy = NULL);

    void addVersionToCache(uintptr_t addr, uint64_t version);

    uint64_t getVersion(uintptr_t addr);

    uint64_t getCachedVersion(uintptr_t addr);

    void invalidate(uintptr_t addr, uint64_t version);

    void setCacheLine(uint32_t pow) { cl_pow=pow; }

    bool inCache(uintptr_t addr);

    inline int size();

	~DSTMGASNetCache();
private:
	gasnett_mutex_t mutex;
	cache_map_t i_map;
	uint32_t cl_pow;
};

#endif /* DSTM_CACHE_H_ */
