/*
 * dstm_cache_impl.h
 *
 *  Created on: 09.02.2012
 *      Author: igor
 */

#ifndef DSTM_CACHE_IMPL_H_
#define DSTM_CACHE_IMPL_H_

#include <gasnet.h>
#include <gasnet_tools.h>

#include "dstm_cache.h"
#include "dstm_lib_p1/dstm_timestamps.h"

#ifdef CACHE
template<class T>
T DSTMGASNetCache::get(uintptr_t addr, uintptr_t lock_addr, uint64_t* version,uintptr_t copy)
{
    //fprintf(stderr,"%s %p\n",__PRETTY_FUNCTION__,addr);
    uintptr_t al = (addr & ADDRESS_MASK) >> cl_pow;
    uintptr_t low_bound = al << cl_pow;

    gasnet_node_t node = ((NODE_MASK << 48) & (addr))>>48;

//    cache_map_t::accessor a_inserted;

    T value;

    /*    if (node == gasnet_mynode()) {
        uint64_t version2;

        do {
            *version = *(uint64_t*)(lock_addr & ADDRESS_MASK);

            value = *(T*)(addr & ADDRESS_MASK);

            version2 = *(uint64_t*)(lock_addr & ADDRESS_MASK);

            if (*version != version2) {
                if ((*version >> 63) || (version2 >> 63)){
                    if((version2 >> 63))
                        *version = version2;
                    break;
                }
            }
        } while (*version != version2);

        if (copy != NULL){

            *(uint64_t*)copy = *version;

            memcpy((void*) (copy + sizeof(uint64_t)), (void*) low_bound,
                   (1 << cl_pow));
        }

        if (!(*version >> 63))
            increaseBRV(*version);

        return value;
    } else {

        //fprintf(stderr,"%s - 0 %p copy %p\n",__PRETTY_FUNCTION__,addr,copy);
        if (i_map.insert(a_inserted, low_bound | ((uintptr_t) node << 48))) {
            uintptr_t new_cl = (uintptr_t) malloc(
                        (1 << cl_pow) + 2*sizeof(uint64_t));

            uint64_t version2;

            do {

                gasnet_handle_t handle;

                gasnet_begin_nbi_accessregion();

                gasnet_get_nbi((void*) (new_cl + sizeof(uint64_t)), node,
                               (void*) (lock_addr & ADDRESS_MASK), sizeof(uint64_t));
                gasnet_get_nbi((void*) (new_cl + 2*sizeof(uint64_t)), node,
                               (void*) low_bound, 1 << cl_pow);

                handle = gasnet_end_nbi_accessregion();

                a_inserted->second = (uintptr_t) new_cl;

                gasnet_wait_syncnb(handle);

                *version = *((uint64_t*) (new_cl + sizeof(uint64_t)));

                gasnet_get((void*) &version2, node,
                           (void*) (lock_addr & ADDRESS_MASK), sizeof(uint64_t));

                if (*version != version2) {
                    if ((*version >> 63) || (version2 >> 63)){
                        if((version2 >> 63)){
                            *version = version2;
                            *((uint64_t*) (new_cl + sizeof(uint64_t))) = version2;
                        }
                        break;
                    }
                }
            } while (*version != version2);

            if (!(*version >> 63)){
                increaseBRV(*version);
                *((uint64_t*) new_cl) = getBRV();
            }

            if (copy != NULL)
                memcpy((void*) copy, (void*) (new_cl + sizeof(uint64_t)),
                       (1 << cl_pow) + sizeof(uint64_t));

            value = *(T*) ((uintptr_t) new_cl + 2*sizeof(uint64_t) + ((addr
                                                                       & ADDRESS_MASK) - low_bound));

            //fprintf(stderr,"%s - 1 %p val %p\n",__PRETTY_FUNCTION__,addr,value);

            return value;
        }

        *version = *((uint64_t*) (a_inserted->second + sizeof(uint64_t)));
        uint64_t brv = *((uint64_t*) a_inserted->second);
        uint64_t cur_brv = getBRV();

        /// invalidate cached object
        if ((*version >> 63) || (brv < cur_brv)) {

            uint64_t version2;

            do {

                gasnet_handle_t handle;

                gasnet_begin_nbi_accessregion();

                gasnet_get_nbi((void*) (a_inserted->second + sizeof(uint64_t)), node,
                               (void*) (lock_addr & ADDRESS_MASK), sizeof(uint64_t));
                gasnet_get_nbi((void*) (a_inserted->second + 2*sizeof(uint64_t)),
                               node, (void*) low_bound, 1 << cl_pow);

                handle = gasnet_end_nbi_accessregion();

                gasnet_wait_syncnb(handle);

                *version = *((uint64_t*) (a_inserted->second + sizeof(uint64_t)));

                gasnet_get((void*) &version2, node,
                           (void*) (lock_addr & ADDRESS_MASK), sizeof(uint64_t));

                if (*version != version2) {
                    if ((*version >> 63) || (version2 >> 63)){
                        if((version2 >> 63)){
                            *version = version2;
                            *((uint64_t*) (a_inserted->second + sizeof(uint64_t))) = version2;
                        }
                        break;
                    }
                }
            } while (*version != version2);
        }

        if (!(*version >> 63)){
            increaseBRV(*version);
            *((uint64_t*) a_inserted->second) = getBRV();

        }

        if (copy != NULL)
            memcpy((void*) copy, (void*) (a_inserted->second + sizeof(uint64_t)),
                   (1 << cl_pow) + sizeof(uint64_t));

        value = *(T*) (a_inserted->second + 2*sizeof(uint64_t) + ((addr
                                                                   & ADDRESS_MASK) - low_bound));
    }
    //fprintf(stderr,"%s - 2 %p val %p\n",__PRETTY_FUNCTION__,addr,value);
*/
    return value;
}

#else

template<class T>
T DSTMGASNetCache::get(uintptr_t addr, uintptr_t lock_addr, uint64_t* version, uintptr_t copy)
{
    uintptr_t al = (addr & ADDRESS_MASK) >> cl_pow;
    uintptr_t low_bound = al << cl_pow;

    gasnet_node_t node = (NODE_MASK & (addr)) >> 48;

    T value = 0;

    uintptr_t new_cl = (uintptr_t) malloc((1 << cl_pow) + sizeof(uint64_t));

    uintptr_t new_val = (uintptr_t) malloc(sizeof(T));

    uint64_t version2;

    if (node == gasnet_mynode()) {

        do {
            *version = *(uint64_t*) (lock_addr & ADDRESS_MASK);

            if (*version >> 63)
                break;

            memcpy((void*) (new_cl + sizeof(uint64_t)), (void*) low_bound,
                   (1 << cl_pow));

            *((uint64_t*)new_cl) = *version;

            version2 = *(uint64_t*) (lock_addr & ADDRESS_MASK);

            if (version2 >> 63) {
                *version = version2;
                *((uint64_t*) new_cl) = version2;
                break;
            }
        } while (*version != version2);

        if (copy != NULL)
            memcpy((void*) copy, (void*) new_cl,
                   (1 << cl_pow) + sizeof(uint64_t));

        value = *(T*) ((uintptr_t) new_cl + sizeof(uint64_t) + ((addr
                                                                 & ADDRESS_MASK) - low_bound));

    } else {

        do {
            gasnet_get((void*) new_cl, node,
                       (void*) (lock_addr & ADDRESS_MASK), sizeof(uint64_t));

            *version = *((uint64_t*) new_cl);

            if (*version >> 63)
                break;

            //			gasnet_handle_t handle;

            //			gasnet_begin_nbi_accessregion();

            if (copy != NULL)
                gasnet_get((void*) (new_cl + sizeof(uint64_t)), node,
                           (void*) low_bound, 1 << cl_pow);
            else
                gasnet_get((void*) new_val , node,
                           (void*) (addr & ADDRESS_MASK), sizeof(T));

            //			handle = gasnet_end_nbi_accessregion();

            //			gasnet_wait_syncnb(handle);

            gasnet_get((void*) &version2, node,
                       (void*) (lock_addr & ADDRESS_MASK), sizeof(uint64_t));

            if ((version2 >> 63)) {
                *version = version2;
                *((uint64_t*) new_cl) = version2;
                break;
            }

        } while (*version != version2);

        if (copy != NULL){
            memcpy((void*) copy, (void*) new_cl,
                   (1 << cl_pow) + sizeof(uint64_t));

            value = *(T*) ((uintptr_t) new_cl + sizeof(uint64_t) + ((addr
                                                                     & ADDRESS_MASK) - low_bound));
        } else
            value = *(T*) new_val;
    }

    free((void*) new_cl);
    free((void*) new_val);

    return value;
}
#endif

template<class T>
T DSTMGASNetCache::readCell(uintptr_t addr, uintptr_t lock_addr, uint64_t* version, uintptr_t copy)
{
    uintptr_t al = ADDR_FROM_ADDRESS(addr) >> cl_pow;
    uintptr_t low_bound = al << cl_pow;

    gasnet_node_t node = NODE_FROM_ADDRESS(addr);

    T value = 0;

    uintptr_t new_cl = (uintptr_t) malloc((1 << cl_pow) + sizeof(uint64_t));

    uintptr_t new_val = (uintptr_t) malloc(sizeof(T));

    uint64_t version2;

    if (node == gasnet_mynode()) {

        do {
	    gasnett_local_mb();
            *version = *(uint64_t*) ADDR_FROM_ADDRESS(lock_addr);

            if (LOCKBIT_TS(*version)) {
                free((void*) new_cl);
                free((void*) new_val);
                return 0;
            }

            if (copy != NULL)
                memcpy((void*) (new_cl + sizeof(uint64_t)), (void*) low_bound, (1 << cl_pow));
            else
                *(T*)new_val = *(T*) ADDR_FROM_ADDRESS(addr);

	    gasnett_local_mb();
            version2 = *(uint64_t*) ADDR_FROM_ADDRESS(lock_addr);

            if (LOCKBIT_TS(version2)) {
                *version = version2;
                free((void*) new_cl);
                free((void*) new_val);
                return 0;
            }
        } while (!isTimestampsEquivalent(*version, version2));

        if (copy != NULL) {
            *((uint64_t*)new_cl) = *version;
            memcpy((void*) copy, (void*) new_cl, (1 << cl_pow) + sizeof(uint64_t));

            value = *(T*) ((uintptr_t) new_cl + sizeof(uint64_t) + (ADDR_FROM_ADDRESS(addr) - low_bound));
        } else
            value = *(T*) new_val;

    } else {

//        int read_nums = 0;
        do {
/*            if (read_nums > 0){
                *version |= (uint64_t)1<<63;
                return 0;
            }*/
            gasnet_get((void*) new_cl, node,
                       (void*) ADDR_FROM_ADDRESS(lock_addr), sizeof(uint64_t));
            *version = *((uint64_t*) new_cl);

            if (LOCKBIT_TS(*version)) {
                free((void*) new_cl);
                free((void*) new_val);
                return 0;
            }

//             gasnet_handle_t handle;

//             gasnet_begin_nbi_accessregion();

            if (copy != NULL)
                gasnet_get((void*) (new_cl + sizeof(uint64_t)), node, (void*) low_bound, 1 << cl_pow);
            else
                gasnet_get((void*) new_val , node, (void*) ADDR_FROM_ADDRESS(addr), sizeof(T));

//             handle = gasnet_end_nbi_accessregion();

//             gasnet_wait_syncnb(handle);

            gasnet_get((void*) &version2, node, (void*) ADDR_FROM_ADDRESS(lock_addr), sizeof(uint64_t));

            if (LOCKBIT_TS(version2)) {
                *version = version2;
                free((void*) new_cl);
                free((void*) new_val);
                return 0;
            }

//            read_nums++;
        } while (!isTimestampsEquivalent(*version, version2));

        if (copy != NULL){
            *((uint64_t*)new_cl) = *version;
            memcpy((void*) copy, (void*) new_cl, (1 << cl_pow) + sizeof(uint64_t));

            value = *(T*) ((uintptr_t) new_cl + sizeof(uint64_t) + (ADDR_FROM_ADDRESS(addr) - low_bound));
        } else
            value = *(T*) new_val;
    }

    free((void*) new_cl);
    free((void*) new_val);

    return value;
}

#endif /* DSTM_CACHE_IMPL_H_ */
