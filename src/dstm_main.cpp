/*
 * main.cpp
 *
 *  Created on: 14.05.2011
 *      Author: igor
 */

#include "gasnet_handlers.h"

#include "dstm_pthread/pass/PthreadCreateReplacePass.h"

#include <string>
#include <iostream>

#include "dstm_gasnet_application.h"
#include "gasnet_thread_spawner.h"
#include "dummy_loadbalancer.h"

#include <gflags/gflags.h>

#include "profiling.h"

#include "dstm_barrier.h"

#ifdef TRANSACTIONS_STATS
#include "dstm_stats.h"
#endif

using namespace llvm;
using namespace std;

DEFINE_int32(pool_size, 8,"Thread pool size on each gasnet node");
DEFINE_string(app, "test.bc", "Name of user application");
DEFINE_string(args, "", "Args of user application");

int main(int argc,char **argv)
{
	gasnet_node_t rank, size;
	gasnet_seginfo_t *segs;

	GASNET_Safe(gasnet_init(&argc,&argv));
	GASNET_Safe(gasnet_attach(htable,HANDLER_TABLE_SIZE,262144*GASNET_PAGESIZE,GASNET_PAGESIZE));

#if defined LOGGING || defined PROFILING
	google::InitGoogleLogging(argv[0]);
#endif

	google::ParseCommandLineFlags(&argc,&argv,true);

	rank = gasnet_mynode();
	size = gasnet_nodes();
	segs = new gasnet_seginfo_t[size];

	GASNET_Safe(gasnet_getSegmentInfo(segs,(int)size));

#ifdef LOGGING
	LOG(INFO)<<"Hello from "<<rank<<" of "<<size<<" | start address -> "<<segs[rank].addr<<" | size -> "<<segs[rank].size;
#endif

	application_ptr_t Application (new DSTMGASNetApplication(FLAGS_app));

	gasnet_handlers_init(Application.get());

	BEGIN_TIME_PROFILING();

//	gasnett_tick_t start;
//	gasnett_tick_t end;
//	start = gasnett_ticks_now();

	if(rank == 0){
		BEGIN_TIME_PROFILING();
        Application->init(true, FLAGS_pool_size);
		END_TIME_PROFILING("main init");

		BARRIER();

		BEGIN_TIME_PROFILING();
		Application->runMainThread();
		END_TIME_PROFILING("main runMainThread");

		BEGIN_TIME_PROFILING();

#ifdef TRANSACTIONS_STATS
		init_nodes_stats(gasnet_nodes());
#endif

		for (int i = 1; i < gasnet_nodes(); i++)
			GASNET_Safe(gasnet_AMRequestShort0(i,hidx_endexec_shrthandler));

		BARRIER();
		END_TIME_PROFILING("main BARRIER");
#ifdef TRANSACTIONS_STATS
		Application.reset();
		GASNET_BLOCKUNTIL(GLOBAL_RETRIVE_STATS == gasnet_nodes());
		print_nodes_stats();
#endif
	}else{
		Application->init(false,FLAGS_pool_size);

		BARRIER();

		GASNET_BLOCKUNTIL(GLOBAL_END_EXECUTION == 1);
		BARRIER();
#ifdef TRANSACTIONS_STATS
		Application.reset();
#endif
	}

//	end = gasnett_ticks_now();
//	uint64_t time_ns = gasnett_ticks_to_ns(end-start);
//	printf("TIME: %f\n",((double)time_ns)/1000000000);

	END_TIME_PROFILING("main TOTAL");

    delete [] segs;

//	Application.reset();

	gasnet_exit(0);

	return 0;
}
