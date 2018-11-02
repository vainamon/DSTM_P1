/*
 * dstm_pthread.h
 *
 *  Created on: 17.08.2011
 *      Author: igor
 */

#ifndef DSTM_PTHREAD_H_
#define DSTM_PTHREAD_H_

#ifdef __cplusplus
extern "C"{
#endif

	typedef unsigned long int dstm_pthread_t;

	/*
	 *  Detach state.
	 */
	enum
	{
	  DSTM_PTHREAD_CREATE_JOINABLE,
	#define DSTM_PTHREAD_CREATE_JOINABLE DSTM_PTHREAD_CREATE_JOINABLE
	  DSTM_PTHREAD_CREATE_DETACHED
	#define DSTM_PTHREAD_CREATE_DETACHED	DSTM_PTHREAD_CREATE_DETACHED
	};

	typedef struct _dstm_pthread_attr_t {

	} dstm_pthread_attr_t;

	/*
	 * pthread wrappers
	 */

	int dstm_pthread_create(dstm_pthread_t *__new_thread,const dstm_pthread_attr_t *__attr,
			char *__start_routine, /// void *(*start_routine) (void*)
			void *__arg);

	int dstm_pthread_join(dstm_pthread_t __th, void **__thread_return);

	int dstm_pthread_attr_init(dstm_pthread_attr_t *__attr);

	int dstm_pthread_attr_destroy(dstm_pthread_attr_t *__attr);

	int dstm_pthread_attr_setdetachstate(dstm_pthread_attr_t *__attr, int __detachstate);

	/*
	 * Global initialization
	 */

	int pthread_init(void *__thread_spawner);

	/*
	 * Misc types
	 */

	typedef struct _newthread{
		dstm_pthread_t *__new_thread;
		dstm_pthread_attr_t __attr;
		char __start_routine[30];
		void *__arg;
	} newthread_t;

	typedef newthread_t* newthread_ptr_t;

#ifdef __cplusplus
}
#endif

#endif /* DSTM_PTHREAD_H_ */
