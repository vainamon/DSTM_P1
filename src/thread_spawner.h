/*
 * thread_spawner.h
 *
 *  Created on: 01.10.2011
 *      Author: igor
 */

#ifndef THREAD_SPAWNER_H_
#define THREAD_SPAWNER_H_

class ThreadSpawner {
public:
	virtual int spawnThread(void*) = 0;
	virtual int joinThread(void*) = 0;
};

typedef ThreadSpawner* threadspawner_ptr_t;

#endif /* THREAD_SPAWNER_H_ */
