/*
 * dstm_dstm_pthread.cpp
 *
 *  Created on: 17.08.2011
 *      Author: igor
 */

#include "dstm_pthread.h"
#include "../thread_spawner.h"
#include <cstdio>
#include <cstring>

threadspawner_ptr_t GLOBAL_THREAD_SPAWNER = 0;

int dstm_pthread_create(dstm_pthread_t *__new_thread, const dstm_pthread_attr_t *__attr,
		char *__start_routine, /// void *(*start_routine) (void*)
		void *__arg)
{
	if (GLOBAL_THREAD_SPAWNER == 0)
		return 1;

	newthread_t nt;

	strcpy(nt.__start_routine, __start_routine);
	nt.__new_thread = __new_thread;
	nt.__arg = __arg;

	//printf("In dstm_pthread: create, address <0x%llx>\n", __new_thread);

	newthread_ptr_t ntp = &nt;
	GLOBAL_THREAD_SPAWNER->spawnThread(ntp);

	return 0;
}

int dstm_pthread_join(dstm_pthread_t __th, void **__thread_return)
{
	//printf("In dstm_pthread: join gtid <%d> \n",__th);
	GLOBAL_THREAD_SPAWNER->joinThread(&__th);
	return 0;
}

int dstm_pthread_attr_init(dstm_pthread_attr_t *__attr)
{
	return 0;
}

int dstm_pthread_attr_destroy(dstm_pthread_attr_t *__attr)
{
	return 0;
}

int dstm_pthread_attr_setdetachstate(dstm_pthread_attr_t *__attr, int __detachstate)
{
	return 0;
}

int pthread_init(void* __thread_spawner)
{
	GLOBAL_THREAD_SPAWNER = threadspawner_ptr_t(__thread_spawner);
}
