/*
 * MemoryManager.h
 *
 *  Created on: 01.03.2012
 *      Author: igor
 */

#ifndef MEMORYMANAGER_H_
#define MEMORYMANAGER_H_

#include <stdint.h>

class MemoryManager
{
public:

	virtual uintptr_t getObjectLockAddr(uintptr_t addr) = 0;

    virtual uintptr_t getObjectLockAddr(uintptr_t addr, uint32_t node) = 0;

	virtual void free(uintptr_t ptr) = 0;

	virtual uintptr_t malloc(size_t size) = 0;
};

#endif /* MEMORYMANAGER_H_ */
