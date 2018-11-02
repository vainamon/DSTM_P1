/*
 * malloc.h
 *
 *  Created on: 14.05.2011
 *      Author: Igor Danilov
 */

#ifndef MALLOC_H_
#define MALLOC_H_

#include <stdint.h>

#define BUDDY_LEVEL 24
#define CDU_SIZE 6
#define MAU_SIZE 3
#define OBJECTS_COUNT ((1<<BUDDY_LEVEL)>>CDU_SIZE)

#define ADDRESS_MASK 0xffffffffffffl
#define NODE_MASK (0x7fffl << 48)

#define NODE_FROM_ADDRESS(x) ((NODE_MASK & (uintptr_t)x) >> 48)
#define ADDR_FROM_ADDRESS(x) (ADDRESS_MASK & (uintptr_t)x)

#ifdef __cplusplus
extern "C"{
#endif

typedef unsigned long size_t;

void dstm_malloc_init(uintptr_t _lb);

void *dstm_malloc(size_t __size, uint32_t __node);

void dstm_free(void *__ptr);

void dstm_malloc_shutdown();

void *dstm_memcpy(void* dst, void* src, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* MALLOC_H_ */
