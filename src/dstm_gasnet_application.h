/*
 * dstm_application.h
 *
 *  Created on: 15.09.2011
 *      Author: igor
 */

#ifndef DSTM_GASNET_APPLICATION_H_
#define DSTM_GASNET_APPLICATION_H_

#include "application.h"

#include "gasnet_common.h"

#include "llvm_common.h"

namespace dstm_pthread
{
	#include "dstm_pthread/dstm_pthread.h"
}

//#include <boost/task.hpp>

#include <boost/unordered_map.hpp>

using namespace llvm;

typedef boost::shared_ptr<ExecutionEngine> exec_engine_ptr;
typedef boost::shared_ptr<LLVMContext> llvm_context_ptr;


class DSTMGASNetApplication : public Application
{
public:
	DSTMGASNetApplication(std::string _bcFile);
	~DSTMGASNetApplication();

	int runMainThread();
	int runChildThread(void* _newThread, int _mainNode, int _gtid);

	void init(bool _main = true, int _poolsize = 8);
protected:
    int getNextLTID() { return ++currentLTID; }

	virtual int addLocalThread(int _gtid,int _ltid,int _state);

	virtual int addRemoteThread(int _gtid,int _ltid,int _state);

	virtual bool inLocalThreads(int _gtid);

	virtual bool inRemoteThreads(int _gtid);

	virtual Application::application_thread_ptr_t getLocalThread(int _gtid);

	virtual Application::application_thread_ptr_t getRemoteThread(int _gtid);

	void runOptPasses(Module*);

	void runTangerPass();

private:
	/// LLVM .bc application filename
	std::string bitcodeFile;
	exec_engine_ptr mainEE;

	Module* userModule;

	/**
	 *  boost threads pool
	 */

	/*typedef boost::tasks::static_pool<boost::tasks::unbounded_fifo> pool_type;
	typedef boost::shared_ptr<pool_type> pool_ptr;

	typedef boost::tasks::handle<void> handle_type;
	typedef boost::shared_ptr<handle_type> handle_ptr;*/

	//pool_ptr applicationPool;

	/// Current local thread identifier
	volatile int currentLTID;

	/// Map of the local application threads. Key - thread global ID
	boost::unordered_map<int,application_thread_ptr_t> localThreads;
	/// Map of the remote application threads. Key - thread global ID
	boost::unordered_map<int,application_thread_ptr_t> remoteThreads;
};

#endif /* DSTM_GASNET_APPLICATION_H_ */
