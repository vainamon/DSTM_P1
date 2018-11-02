/*
 * gasnet_thread_spawner.cpp
 *
 *  Created on: 01.10.2011
 *      Author: igor
 */

#include "gasnet_thread_spawner.h"
#include "gasnet_handlers.h"
#include "profiling.h"

int GASNetThreadSpawner::spawnThread(void* _newThread)
{
	BEGIN_TIME_PROFILING();

	int gtid = this->getNextGTID();

	*dstm_pthread::newthread_ptr_t(_newThread)->__new_thread = gtid;

	int mynode = gasnet_mynode();
	int victim = loadBalancer->getVictim();

	if (victim == mynode){
        mainApplication->addLocalThread(gtid, 0, Application::RUNNABLE);
	} else {
        mainApplication->addRemoteThread(gtid, 0, Application::RUNNABLE);
	}

	START_HANDLER_TIME_PROFILING(spawnthread,gtid);

	GASNET_Safe(gasnet_AMRequestMedium1
                    (victim, hidx_spawnthread_medhandler, _newThread, sizeof(dstm_pthread::newthread_t), gtid));

	END_TIME_PROFILING("");

	return 0;
}

int GASNetThreadSpawner::joinThread(void* _thr)
{
	BEGIN_TIME_PROFILING();

	dstm_pthread::dstm_pthread_t* thr_ptr = (dstm_pthread::dstm_pthread_t*)_thr;

	if(mainApplication->inLocalThreads(*thr_ptr)){
#ifdef LOGGING
	VLOG(3)<<"Inside function "<<__PRETTY_FUNCTION__<<" thread: gtid ="<<*thr_ptr<<" state = "<<mainApplication->getLocalThread(*thr_ptr)->state;
#endif
		GASNET_BLOCKUNTIL(mainApplication->getLocalThread(*thr_ptr)->state == Application::EXITED);
	}

	if(mainApplication->inRemoteThreads(*thr_ptr)){
#ifdef LOGGING
	VLOG(3)<<"Inside function "<<__PRETTY_FUNCTION__<<" thread: gtid ="<<*thr_ptr<<" state = "<<mainApplication->getRemoteThread(*thr_ptr)->state;
#endif
		GASNET_BLOCKUNTIL(mainApplication->getRemoteThread(*thr_ptr)->state == Application::EXITED);
	}

	END_TIME_PROFILING("");

	return 0;
}

int GASNetThreadSpawner::getNextGTID()
{
	return gtid++;
}
