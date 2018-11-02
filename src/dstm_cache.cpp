/*
 * dstm_cache.cpp
 *
 *  Created on: 13.01.2012
 *      Author: igor
 */

#include "dstm_cache.h"

#include "dstm_cache_impl.h"

template uint32_t DSTMGASNetCache::get<uint32_t>(uintptr_t,uintptr_t,uint64_t*,uintptr_t);
template uint64_t DSTMGASNetCache::get<uint64_t>(uintptr_t,uintptr_t,uint64_t*,uintptr_t);
template uint32_t DSTMGASNetCache::readCell<uint32_t>(uintptr_t,uintptr_t,uint64_t*,uintptr_t);
template uint64_t DSTMGASNetCache::readCell<uint64_t>(uintptr_t,uintptr_t,uint64_t*,uintptr_t);

DSTMGASNetCache::DSTMGASNetCache() : cl_pow(CDU_SIZE)
{
    gasnett_mutex_init(&mutex);
}

DSTMGASNetCache::~DSTMGASNetCache()
{
}

void DSTMGASNetCache::addVersionToCache(uintptr_t addr, uint64_t version)
{
    //    cache_map_t::accessor a_object;

    if (LOCKBIT_TS(version))
        return;
    
    /*    if (i_map.insert(a_object, addr)) {
        a_object->second = version;
    } else {
        if ( isTimestampGreater(version, a_object->second) || (LOCKBIT_TS(version) && isTimestampsEqual(version, a_object->second)) )
            a_object->second = version;
    }*/
    gasnett_mutex_lock(&mutex);
    if(i_map.find(addr) != i_map.end())
    {
        if ( isTimestampGreater(version, i_map[addr]) || (LOCKBIT_TS(version) && isTimestampsEqual(version, i_map[addr])) )
            i_map[addr] = version;
    }
    else
        i_map[addr] = version;
    gasnett_mutex_unlock(&mutex);
}

uint64_t DSTMGASNetCache::getCachedVersion(uintptr_t addr)
{
    /*cache_map_t::const_accessor a_object;

    if (i_map.find(a_object, addr))
        return a_object->second;*/
    gasnett_mutex_lock(&mutex);

    if(i_map.find(addr) != i_map.end())
{
	gasnett_mutex_unlock(&mutex);
        return i_map[addr];
}
    gasnett_mutex_unlock(&mutex);

    return 0;
}

uint64_t DSTMGASNetCache::getVersion(uintptr_t addr)
{
#ifdef CACHE
    /*    cache_map_t::const_accessor a_object;

    if (i_map.find(a_object,addr)){
        uint64_t version = *((uint64_t*)(a_object->second + sizeof(uint64_t)));
        return version;
    }*/
    uint64_t version;
    gasnet_node_t node = NODE_FROM_ADDRESS(addr);

    gasnet_get((void*) &version, node, (void*) ADDR_FROM_ADDRESS(addr), sizeof(uint64_t));

    return version;
#else
    uint64_t version;
    gasnet_node_t node = NODE_FROM_ADDRESS(addr);

    gasnet_get((void*) &version, node, (void*) ADDR_FROM_ADDRESS(addr), sizeof(uint64_t));

    return version;
#endif
}

void  DSTMGASNetCache::invalidate(uintptr_t addr, uint64_t version)
{
#ifdef CACHE
    /*    cache_map_t::accessor a_object;

    if (i_map.find(a_object,addr)){

        if ( (*((uint64_t*)(a_object->second + sizeof(uint64_t))) >> 63) || (*((uint64_t*)(a_object->second + sizeof(uint64_t))) > version) )
            return;
        else
            *((uint64_t*)(a_object->second + sizeof(uint64_t))) |= (((uint64_t) 1) << 63 );
    }*/
#endif
}

bool DSTMGASNetCache::inCache(uintptr_t addr)
{
/*    cache_map_t::const_accessor a_object;

    if (i_map.find(a_object,addr))
        return true;*/

    return false;
}

int DSTMGASNetCache::size()
{
    return i_map.size();
}
