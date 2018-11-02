/*
 * buddy.h
 *
 *  Created on: 25.01.2012
 *      Author: igor
 */

#ifndef BUDDY_MEMORY_ALLOCATION_H
#define BUDDY_MEMORY_ALLOCATION_H

#ifdef __cplusplus
extern "C"{
#endif

struct buddy;

struct buddy * buddy_new(int level);
void buddy_delete(struct buddy *);
int buddy_alloc(struct buddy *, int size);
void buddy_free(struct buddy *, int offset);
int buddy_size(struct buddy *, int offset);
void buddy_dump(struct buddy *);

#ifdef __cplusplus
}
#endif

#endif
