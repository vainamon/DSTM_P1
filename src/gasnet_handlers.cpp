/*
 * gasnet_handlers.cpp
 *
 *  Created on: 12.09.2011
 *      Author: igor
 */

#include "gasnet_handlers.h"
#include "dstm_gasnet_appl_wrapper.h"
#include "profiling.h"

#include "glue.h"

#include "dstm_malloc/dstm_malloc.h"

#include <gasnet_handler.h>

namespace dstm_pthread
{
	#include "dstm_pthread/dstm_pthread.h"
}

Application* GLOBAL_APPLICATION;

int GLOBAL_END_EXECUTION = 0;

#ifdef TRANSACTIONS_STATS

#include "dstm_stats.h"

volatile int GLOBAL_RETRIVE_STATS = 0;

#endif

DEFINE_HANDLER_TIME_PROFILING(spawnthread, 1)

int
gasnet_handlers_init(Application* __application)
{
	GLOBAL_APPLICATION = __application;
}

typedef void(*handler_func)();

gasnet_handlerentry_t htable[] = {
        { hidx_endexec_shrthandler,				(handler_func)endexec_shrthandler			},
        { hidx_endexec_succ_shrthandler,		(handler_func)endexec_succ_shrthandler		},
        { hidx_spawnthread_medhandler,			(handler_func)spawnthread_medhandler		},
        { hidx_spawnthread_succ_medhandler,		(handler_func)spawnthread_succ_medhandler	},
        { hidx_threadexited_shrthandler,		(handler_func)threadexited_shrthandler		},
        { hidx_threadexited_succ_shrthandler,	(handler_func)threadexited_succ_shrthandler	},
        { hidx_requestlocks_medhandler,			(handler_func)requestlocks_medhandler		},
        { hidx_requestlocks_succ_medhandler,	(handler_func)requestlocks_succ_medhandler	},
        { hidx_dstmfree_medhandler,				(handler_func)dstmfree_medhandler			},
        { hidx_dstmfree_succ_medhandler,		(handler_func)dstmfree_succ_medhandler		},
        { hidx_dstmmalloc_shrthandler,			(handler_func)dstmmalloc_shrthandler		},
        { hidx_dstmmalloc_succ_shrthandler,		(handler_func)dstmmalloc_succ_shrthandler	},
        { hidx_dstmstats_medhandler,			(handler_func)dstmstats_medhandler			},
        { hidx_dstmstats_succ_medhandler,		(handler_func)dstmstats_succ_medhandler		},
};

void
spawnthread_medhandler (gasnet_token_t token, void *buf, size_t nbytes, harg_t gtid)
{
	BEGIN_TIME_PROFILING();

	gasnet_node_t   node;
	gasnet_AMGetMsgSource(token, &node);

	int ltid = 0;

	if(GLOBAL_APPLICATION != 0){
        ltid = GLOBAL_APPLICATION->runChildThread(buf, node, gtid);
	}

	GASNET_Safe(gasnet_AMReplyMedium1
                    (token, hidx_spawnthread_succ_medhandler, buf, nbytes, ltid));

	END_TIME_PROFILING("");
}

void
spawnthread_succ_medhandler (gasnet_token_t token, void *buf, size_t nbytes, harg_t ltid)
{
	BEGIN_TIME_PROFILING();

	gasnet_node_t   node;
	gasnet_AMGetMsgSource(token, &node);

#ifdef LOGGING
    VLOG(3)<<"Inside function "<<__PRETTY_FUNCTION__<<" from node = "<<node<<" ltid = "<<ltid;
#endif

	int gtid = *dstm_pthread::newthread_ptr_t(buf)->__new_thread;

	DSTMGASNetApplWrapper* applWrp = (DSTMGASNetApplWrapper*)(GLOBAL_APPLICATION);

	Application::application_thread_ptr_t thread_ptr;

	if(applWrp->inLocalThreads(gtid))
		thread_ptr = applWrp->getLocalThread(gtid);
	else
		if(applWrp->inRemoteThreads(gtid))
			thread_ptr = applWrp->getRemoteThread(gtid);

	thread_ptr->ltid = ltid;

	if(thread_ptr->state == Application::RUNNABLE)
		thread_ptr->state = Application::RUNNING;

    END_HANDLER_TIME_PROFILING(spawnthread, gtid);

	END_TIME_PROFILING("");

}

void
threadexited_shrthandler (gasnet_token_t token, harg_t gtid, harg_t state)
{
#ifdef LOGGING
	VLOG(3)<<"Inside function "<<__PRETTY_FUNCTION__<<" GTID = "<<gtid<<" state = "<<state;
#endif

	BEGIN_TIME_PROFILING();

	gasnet_node_t   node;
	gasnet_AMGetMsgSource(token, &node);

	DSTMGASNetApplWrapper* applWrp = (DSTMGASNetApplWrapper*)(GLOBAL_APPLICATION);

	if(applWrp->inLocalThreads(gtid)){
		applWrp->getLocalThread(gtid)->state = state;
	}else{
		applWrp->getRemoteThread(gtid)->state = state;
	}

	GASNET_Safe(gasnet_AMReplyShort2
					(token, hidx_threadexited_succ_shrthandler, gtid, state));

	END_TIME_PROFILING("");
}

void
threadexited_succ_shrthandler (gasnet_token_t token, harg_t gtid, harg_t state)
{
}

void
requestlocks_medhandler(gasnet_token_t token, void* buf, size_t nbytes)
{
	uint64_t* objects_versions = (uint64_t*)buf;

    uint64_t* result = glue::processLocksRequest(objects_versions, nbytes);

	if(result == NULL)
		GASNET_Safe(gasnet_AMReplyMedium1
                    (token, hidx_requestlocks_succ_medhandler, buf, sizeof(uintptr_t), 0));
	else {
		/// first element - tx descriptor
		result[0] = *((uint64_t*)buf);

		GASNET_Safe(gasnet_AMReplyMedium1
                    (token, hidx_requestlocks_succ_medhandler, result, (result[1]+2)*sizeof(uint64_t), result[1]));
		free(result);
	}
}

void
requestlocks_succ_medhandler(gasnet_token_t, void* buf, size_t, harg_t nobjs)
{
	uint64_t *tx = (uint64_t*)buf;
	uint64_t *changedObjects = (uint64_t*)buf+2;

    glue::locksRequestProcessed((uintptr_t)*tx, changedObjects, nobjs);
}

void
dstmfree_medhandler(gasnet_token_t token, void* buf, size_t nbytes)
{
	glue::dstm_mallocFree(*((uint64_t*)buf));
	GASNET_Safe(gasnet_AMReplyMedium0(token,hidx_dstmfree_succ_medhandler,NULL,0));
}

void
dstmfree_succ_medhandler(gasnet_token_t token, void* buf, size_t nbytes)
{
}

void
dstmmalloc_shrthandler(gasnet_token_t token, harg_t size,
		harg_t remote_ptr_h, harg_t remote_ptr_l, harg_t remote_mutex_h,
		harg_t remote_mutex_l) {

	uintptr_t ptr;

	ptr = glue::dstm_mallocMalloc(size);

/*	fprintf(stderr, "%s size %d _ptr %p mutex %p ptr %p\n", __PRETTY_FUNCTION__, size,
			UNPACK2(remote_ptr_h, remote_ptr_l),
			UNPACK2(remote_mutex_h, remote_mutex_l),
			ptr);
*/
	GASNET_Safe(gasnet_AMReplyShort6(token,hidx_dstmmalloc_succ_shrthandler,
			remote_ptr_h,remote_ptr_l,remote_mutex_h,remote_mutex_l,
			GASNETI_HIWORD(ptr),GASNETI_LOWORD(ptr)));
}

void
dstmmalloc_succ_shrthandler(gasnet_token_t, harg_t ptr_h, harg_t ptr_l,
		harg_t mutex_h, harg_t mutex_l, harg_t returned_ptr_h,
		harg_t returned_ptr_l) {
	uintptr_t* ptr = (uintptr_t*)UNPACK2(ptr_h,ptr_l);
	*ptr = (uintptr_t)UNPACK2(returned_ptr_h,returned_ptr_l);
/*
	fprintf(stderr, "%s _ptr %p mutex %p ptr %p\n", __PRETTY_FUNCTION__,
			UNPACK2(ptr_h, ptr_l),
			UNPACK2(mutex_h, mutex_l),
			*ptr);
*/
	uintptr_t *mtx = (uintptr_t*)UNPACK2(mutex_h,mutex_l);
	*mtx = 1;
	//gasnett_mutex_unlock((gasnett_mutex_t*) UNPACK2(mutex_h,mutex_l));
}

void
endexec_shrthandler(gasnet_token_t token)
{
	GLOBAL_END_EXECUTION = 1;
	GASNET_Safe(gasnet_AMReplyShort0(token,hidx_endexec_succ_shrthandler));
}

void
endexec_succ_shrthandler(gasnet_token_t token)
{
}

void
dstmstats_medhandler(gasnet_token_t token, void* buf, size_t nbytes)
{
#ifdef TRANSACTIONS_STATS
	gasnet_node_t   node;
	gasnet_AMGetMsgSource(token, &node);

	add_node_to_stat(node,*(node_stats_t*)buf);
	GLOBAL_RETRIVE_STATS++;
#endif
}

void
dstmstats_succ_medhandler(gasnet_token_t token, void* buf, size_t nbytes)
{
}
