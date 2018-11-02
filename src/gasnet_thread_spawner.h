/*
 * gasnet_thread_spawner.h
 *
 *  Created on: 01.10.2011
 *      Author: igor
 */

#ifndef GASNET_THREAD_SPAWNER_H_
#define GASNET_THREAD_SPAWNER_H_

#include "thread_spawner.h"
#include "loadbalancer.h"

namespace dstm_pthread
{
	#include "dstm_pthread/dstm_pthread.h"
}

#include "dstm_gasnet_appl_wrapper.h"

class GASNetThreadSpawner : public ThreadSpawner
{
public:
    GASNetThreadSpawner(loadbalancer_ptr_t _lb, DSTMGASNetApplWrapper* _ma) : loadBalancer(_lb), mainApplication(_ma) ,gtid(1) {}

	int spawnThread(void* _newThread);

	int joinThread(void*);

	int getNextGTID();
private:
	loadbalancer_ptr_t loadBalancer;
	DSTMGASNetApplWrapper* mainApplication;
	volatile int gtid;
};

#endif /* GASNET_THREAD_SPAWNER_H_ */
