/*
 * dstm_application.cpp
 *
 *  Created on: 15.09.2011
 *      Author: igor
 */

#include "dstm_gasnet_application.h"
#include "gasnet_handlers.h"
#include "dummy_loadbalancer.h"
#include "gasnet_thread_spawner.h"

#include "tanger/lib/tanger.h"

#include "profiling.h"

DSTMGASNetApplication::DSTMGASNetApplication(std::string _bcFile) : bitcodeFile(_bcFile), currentLTID(0)
{
}

namespace llvm{
ModulePass* createTangerPass(){
    return new tanger::TangerTransform();
}
}

void DSTMGASNetApplication::runOptPasses(Module* module)
{
    BEGIN_TIME_PROFILING();

    PassManager Passes;

    Passes.add(new TargetData(module));
    //	Passes.add(createInternalizePass(true));
    //	Passes.add(createPromoteMemoryToRegisterPass());
    //	Passes.add(createSimplifyLibCallsPass());
    Passes.add(createFunctionInliningPass());
    //	Passes.add(createAlwaysInlinerPass());
    //	Passes.add(createGlobalOptimizerPass());
    //	Passes.add(createGlobalDCEPass());
    //	Passes.add(createCFGSimplificationPass());

    Passes.add(createVerifierPass());

    Passes.run(*module);

    END_TIME_PROFILING("LLVM passes profiling");
}

void DSTMGASNetApplication::runTangerPass()
{
    BEGIN_TIME_PROFILING();

    PassManager Passes;

    Passes.add(new TargetData(userModule));

    Passes.add(createTangerPass());

    Passes.add(createVerifierPass());

    Passes.run(*userModule);

    END_TIME_PROFILING("LLVM Tanger pass profiling");
}

void DSTMGASNetApplication::init(bool _main,int _poolsize)
{
    InitializeNativeTarget();
    llvm_start_multithreaded();

    std::string error;

    MemoryBuffer *mbuf = MemoryBuffer::getFile(bitcodeFile);

    userModule = ParseBitcodeFile(mbuf,
                                  llvm::getGlobalContext(), &error);

    delete mbuf;

    runTangerPass();

    llvm::Linker linker("applLinker", userModule, llvm::Linker::QuietWarnings);
    linker.addSystemPaths();

    const std::string& filename1 = "dstm_pthread";
    const std::string& filename2 = "dstm_lib_p1.bc";
    const std::string& filename3 = "dstm_malloc";

    bool native = true;

    Module *linkedModule;

    Module* dstm_lib_p1;

    dstm_lib_p1 = ParseBitcodeFile(MemoryBuffer::getFile(filename2),
                                   llvm::getGlobalContext(), &error);

    if (linker.LinkInModule(dstm_lib_p1, &error)) {
        outs() << "Link error! dstm_lib_p1.bc LinkModules "<<error<<"\n";
    }

    delete dstm_lib_p1;

    linkedModule = linker.getModule();

    if (linkedModule == NULL) {
        outs() << "Link error! getModule\n";
    }

    runOptPasses(linkedModule);

    if (linker.LinkInLibrary(filename1, native)) {
        outs() << "Link error! dstm_pthread LinkInLibrary\n";
    }

    //	Module* dstm_malloc;
    //
    //	dstm_malloc = ParseBitcodeFile(MemoryBuffer::getFile(filename3),
    //			llvm::getGlobalContext(), &error);

    if (linker.LinkInLibrary(filename3, native)) {
        outs() << "Link error! dstm_malloc LinkInLibrary\n";
    }

    //    outs()<<*linkedModule<<"\n\n---------\n";

    exec_engine_ptr _mainEE(
                llvm::ExecutionEngine::createJIT(linkedModule, &error));
    //exec_engine_ptr _mainEE(llvm::ExecutionEngine::create(linkedModule,true,&error));

    mainEE = _mainEE;

    linker.releaseModule();

    mainEE->runStaticConstructorsDestructors(false);

    //pool_ptr poolPtr(new pool_type(boost::tasks::poolsize(_poolsize)/*pool_type::bind_to_processors()*/));
    //applicationPool = poolPtr;
}

typedef struct thread_arg{
    exec_engine_ptr _childEE;
    int _mainNode;
    int _gtid;
    std::string _func;
    void *_arg;
} thread_arg_t;

typedef struct main_thread_arg{
    exec_engine_ptr _mainEE;
    DSTMGASNetApplication *_app;
} main_thread_arg_t;

std::vector<thread_arg_t*> thread_args;
main_thread_arg_t* main_thread_args;

DSTMGASNetApplication::~DSTMGASNetApplication()
{
    mainEE->runStaticConstructorsDestructors(true);
    llvm_stop_multithreaded();
    for(int i = 0; i<thread_args.size();i++)
        delete thread_args[i];

    delete main_thread_args;
}

bool MAIN_EXITED = false;

void *jobMain(void *t_arg)
{
#ifdef LOGGING
    VLOG(2)<<"inside function "<<__PRETTY_FUNCTION__;
#endif

    BEGIN_TIME_PROFILING();

    std::vector<GenericValue> noargs;
    GenericValue ret;

    main_thread_arg_t* targs = (main_thread_arg_t*)t_arg;

    exec_engine_ptr _mainEE = targs->_mainEE;
    DSTMGASNetApplication *_mainAppl = targs->_app;

    if (_mainEE.get() != NULL){

        typedef void (*PFN)(void*);

        std::vector<GenericValue> args;

        DummyLoadBalancer dummyLB(gasnet_nodes());
        GASNetThreadSpawner spwn(&dummyLB,(DSTMGASNetApplWrapper*)_mainAppl);
        GenericValue val;
        val.PointerVal = (void*)&spwn;

        args.push_back(val);

        BEGIN_TIME_PROFILING();

        _mainEE->runFunction(_mainEE->FindFunctionNamed("pthread_init"),args);

        END_TIME_PROFILING("pthread_init");

        Function *func = _mainEE->FindFunctionNamed("main");

        PFN pfn;

        BEGIN_TIME_PROFILING();

        pfn = reinterpret_cast<PFN>(_mainEE->getPointerToFunction(func));

        END_TIME_PROFILING("JIT-time of main function");

        BEGIN_TIME_PROFILING();

        pfn(NULL);

        END_TIME_PROFILING("exec-time of main function");

        _mainEE->freeMachineCodeForFunction(func);
        //ret = _mainEE->runFunction(_mainEE->FindFunctionNamed("main"),noargs);
    }

    END_TIME_PROFILING("");

    MAIN_EXITED = true;
}

/*void jobMain(exec_engine_ptr _mainEE, DSTMGASNetApplication* _mainAppl)
{
#ifdef LOGGING
    VLOG(3)<<"inside function "<<__PRETTY_FUNCTION__;
#endif

    BEGIN_TIME_PROFILING();

    std::vector<GenericValue> noargs;
    GenericValue ret;

    if (_mainEE.get() != NULL){

        typedef void (*PFN)(void*);

        std::vector<GenericValue> args;

        DummyLoadBalancer dummyLB(gasnet_nodes());
        GASNetThreadSpawner spwn(&dummyLB,(DSTMGASNetApplWrapper*)_mainAppl);
        GenericValue val;
        val.PointerVal = (void*)&spwn;

        args.push_back(val);

        BEGIN_TIME_PROFILING();

        _mainEE->runFunction(_mainEE->FindFunctionNamed("pthread_init"),args);

        END_TIME_PROFILING("pthread_init");

        Function *func = _mainEE->FindFunctionNamed("main");

        PFN pfn;

        BEGIN_TIME_PROFILING();

        pfn = reinterpret_cast<PFN>(_mainEE->getPointerToFunction(func));

        END_TIME_PROFILING("JIT-time of main function");

        BEGIN_TIME_PROFILING();

        pfn(NULL);

        END_TIME_PROFILING("exec-time of main function");

        //ret = _mainEE->runFunction(_mainEE->FindFunctionNamed("main"),noargs);
    }

    END_TIME_PROFILING("");

    MAIN_EXITED = true;
}*/

/*int DSTMGASNetApplication::runMainThread()
{
    handle_ptr mainHandlePtr(new handle_type(boost::tasks::async(
                boost::tasks::make_task(jobMain,mainEE,this),
                    applicationPool boost::tasks::new_thread())));

    addLocalThread(0, 0, RUNNING);

    GASNET_BLOCKUNTIL(MAIN_EXITED == true);

    mainHandlePtr->wait();

    return 0;
}*/

int DSTMGASNetApplication::runMainThread()
{
    main_thread_args = new main_thread_arg_t;

    main_thread_args->_mainEE = mainEE;

    main_thread_args->_app = this;

    pthread_t thr;
    pthread_create(&thr,NULL,jobMain, main_thread_args);

    addLocalThread(0, 0, RUNNING);

    GASNET_BLOCKUNTIL(MAIN_EXITED == true);

    void *res;

    pthread_join(thr, &res);

    return 0;
}

typedef void (*PFN)(void*);

PFN pfn_c = NULL;

void* jobChild(void *t_arg)
{
    thread_arg_t* targs = (thread_arg_t*)t_arg;
#ifdef LOGGING
    VLOG(2)<<"Inside function "<<__PRETTY_FUNCTION__<<" GTID = "<<targs->_gtid;
#endif
    BEGIN_TIME_PROFILING();

    std::vector<GenericValue> noargs;
    std::vector<GenericValue> args;
    GenericValue ret;

    exec_engine_ptr _childEE = targs->_childEE;
    int _mainNode = targs->_mainNode;
    int _gtid = targs->_gtid;
    std::string _func = targs->_func;
    void *_arg = targs->_arg;

    if (_childEE.get() != NULL) {
        if (pfn_c == NULL) {
            Function *func = _childEE->FindFunctionNamed(_func.c_str());

            if (func != NULL) {
                BEGIN_TIME_PROFILING();

                pfn_c = reinterpret_cast<PFN> (_childEE->getPointerToFunction(
                                                   func));

                END_TIME_PROFILING("JIT-time of child function");
            } else
                fprintf(stderr,
                        "ERROR: can't find a child function '%s' in module!\n",
                        _func.c_str());
        }

        if (pfn_c != NULL)
            pfn_c(_arg);
        else
            fprintf(stderr,"ERROR: pointer to child function == NULL!\n");

        //		GenericValue val;
        //		val.PointerVal = (void*)&_arg;
        //
        //		args.push_back(val);
        //
        //		_childEE->runFunction(_childEE->FindFunctionNamed(_func),args);

        //_childEE->runFunction(_childEE->FindFunctionNamed("tanger_stm_thread_shutdown"),noargs);
        //tanger_stm_thread_shutdown();
    }

    GASNET_Safe(gasnet_AMRequestShort2
                (_mainNode, hidx_threadexited_shrthandler, _gtid, Application::EXITED));

    END_TIME_PROFILING(_gtid);

}
/*
void jobChild(exec_engine_ptr _childEE, int _mainNode, int _gtid, std::string _func, void *_arg)
{
#ifdef LOGGING
    VLOG(3)<<"Inside function "<<__PRETTY_FUNCTION__<<" GTID = "<<_gtid;
#endif
    BEGIN_TIME_PROFILING();

    std::vector<GenericValue> noargs;
    std::vector<GenericValue> args;
    GenericValue ret;

    if (_childEE.get() != NULL) {
        if (pfn_c == NULL) {
            Function *func = _childEE->FindFunctionNamed(_func.c_str());

            if (func != NULL) {
                BEGIN_TIME_PROFILING();

                pfn_c = reinterpret_cast<PFN> (_childEE->getPointerToFunction(
                        func));

                END_TIME_PROFILING("JIT-time of child function");
            } else
                fprintf(stderr,
                        "ERROR: can't find a child function '%s' in module!\n",
                        _func.c_str());
        }

        if (pfn_c != NULL)
            pfn_c(_arg);
        else
            fprintf(stderr,"ERROR: pointer to child function == NULL!\n");

//		GenericValue val;
//		val.PointerVal = (void*)&_arg;
//
//		args.push_back(val);
//
//		_childEE->runFunction(_childEE->FindFunctionNamed(_func),args);

        //_childEE->runFunction(_childEE->FindFunctionNamed("tanger_stm_thread_shutdown"),noargs);
        //tanger_stm_thread_shutdown();
    }

    GASNET_Safe(gasnet_AMRequestShort2
                        (_mainNode, hidx_threadexited_shrthandler, _gtid, Application::EXITED));

    END_TIME_PROFILING(_gtid);

}*/

int DSTMGASNetApplication::runChildThread(void* _newThread, int _mainNode, int _gtid)
{
#ifdef LOGGING
    VLOG(2)<<"Inside function "<<__PRETTY_FUNCTION__<<" GTID = "<<_gtid;
#endif

    int ltid;

    BEGIN_TIME_PROFILING();

    std::string startRoutine(dstm_pthread::newthread_ptr_t(_newThread)->__start_routine);
    void *arg = dstm_pthread::newthread_ptr_t(_newThread)->__arg;

    ltid = getNextLTID();
    int mynode = gasnet_mynode();

    if (mynode != _mainNode)
        addLocalThread(_gtid, ltid, RUNNING);

    thread_arg_t* targs = new thread_arg_t;

    targs->_arg = arg;
    targs->_childEE = mainEE;
    targs->_func = startRoutine;
    targs->_gtid = _gtid;
    targs->_mainNode = _mainNode;

    thread_args.push_back(targs);
    pthread_t thr;
    pthread_create(&thr,NULL,jobChild,targs);

    END_TIME_PROFILING("");

    return ltid;
}
/*
int DSTMGASNetApplication::runChildThread(void* _newThread, int _mainNode, int _gtid)
{
#ifdef LOGGING
    VLOG(3)<<"Inside function "<<__PRETTY_FUNCTION__<<" GTID = "<<_gtid;
#endif

    int ltid;

    BEGIN_TIME_PROFILING();

    std::string startRoutine(dstm_pthread::newthread_ptr_t(_newThread)->__start_routine);
    void *arg = dstm_pthread::newthread_ptr_t(_newThread)->__arg;

    ltid = getNextLTID();
    int mynode = gasnet_mynode();

    if (mynode != _mainNode)
        addLocalThread(_gtid, ltid, RUNNING);

    handle_ptr childHandlePtr(new handle_type(boost::tasks::async(
                    boost::tasks::make_task(jobChild,mainEE,_mainNode,_gtid,startRoutine,arg),
                    *applicationPool boost::tasks::new_thread())));

    END_TIME_PROFILING("");

    return ltid;
}*/

gasnett_mutex_t G_THREADS_MUTEX = GASNETT_MUTEX_INITIALIZER;

int DSTMGASNetApplication::addLocalThread(int _gtid,int _ltid,int _state)
{
    application_thread_ptr_t localThread(new ApplicationThread);

    localThread->ltid = _ltid;
    localThread->state = _state;

    gasnett_mutex_lock(&G_THREADS_MUTEX);

    localThreads[_gtid] = localThread;

    gasnett_mutex_unlock(&G_THREADS_MUTEX);

    return 0;
}

int DSTMGASNetApplication::addRemoteThread(int _gtid,int _ltid,int _state)
{
    application_thread_ptr_t remoteThread(new ApplicationThread);

    remoteThread->ltid = _ltid;
    remoteThread->state = _state;

    gasnett_mutex_lock(&G_THREADS_MUTEX);

    remoteThreads[_gtid] = remoteThread;

    gasnett_mutex_unlock(&G_THREADS_MUTEX);

    return 0;
}

bool DSTMGASNetApplication::inLocalThreads(int _gtid)
{
    bool result;

    gasnett_mutex_lock(&G_THREADS_MUTEX);
    result = localThreads.find(_gtid) != localThreads.end() ? true : false;
    gasnett_mutex_unlock(&G_THREADS_MUTEX);

    return result;
}

bool DSTMGASNetApplication::inRemoteThreads(int _gtid)
{
    bool result;

    gasnett_mutex_lock(&G_THREADS_MUTEX);
    result = remoteThreads.find(_gtid) != remoteThreads.end() ? true : false;
    gasnett_mutex_unlock(&G_THREADS_MUTEX);

    return result;
}

Application::application_thread_ptr_t DSTMGASNetApplication::getLocalThread(int _gtid){
    Application::application_thread_ptr_t result;

    gasnett_mutex_lock(&G_THREADS_MUTEX);
    result = localThreads[_gtid];
    gasnett_mutex_unlock(&G_THREADS_MUTEX);

    return result;
}

Application::application_thread_ptr_t DSTMGASNetApplication::getRemoteThread(int _gtid){
    Application::application_thread_ptr_t result;

    gasnett_mutex_lock(&G_THREADS_MUTEX);
    result = remoteThreads[_gtid];
    gasnett_mutex_unlock(&G_THREADS_MUTEX);

    return result;
}
