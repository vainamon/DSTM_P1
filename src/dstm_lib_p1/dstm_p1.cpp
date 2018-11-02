#include "dstm_p1.h"

#include "../dstm_malloc/dstm_malloc.h"
#include "../profiling.h"

#include "../glue.h"

#define GASNET_Safe(fncall) do {                                     \
    int _retval;                                                     \
    if ((_retval = fncall) != GASNET_OK) {                           \
      fprintf(stderr, "ERROR calling: %s\n"                          \
                   " at: %s:%i\n"                                    \
                   " error: %s (%s)\n",                              \
              #fncall, __FILE__, __LINE__,                           \
              gasnet_ErrorName(_retval), gasnet_ErrorDesc(_retval)); \
      fflush(stderr);                                                \
      gasnet_exit(_retval);                                          \
    }                                                                \
  } while(0)

GASNETI_THREADKEY_DEFINE(TX_DATA_THREADKEY);

DSTMGASNetCache *globalCache = NULL;
GASNetMemoryManager *globalMemMgr = NULL;

void initCache()
{
#ifdef LOGGING
    VLOG(0)<<__PRETTY_FUNCTION__<<" CDU_SIZE = "<<CDU_SIZE<<"\n";
#endif
    globalCache = new DSTMGASNetCache();
	((DSTMGASNetCache*)globalCache)->setCacheLine(CDU_SIZE);
}

void initMemory()
{
#ifdef LOGGING
    VLOG(0)<<__PRETTY_FUNCTION__<<"\n";
#endif
    globalMemMgr = new GASNetMemoryManager();

	gasnet_node_t size;
	gasnet_seginfo_t *segs;

	size = gasnet_nodes();
	segs = new gasnet_seginfo_t[size];

	GASNET_Safe(gasnet_getSegmentInfo(segs,(int)size));

	globalMemMgr->init(segs,size);

	dstm_malloc_init(globalMemMgr->getObjsAddr(gasnet_mynode()));

    delete [] segs;

	glue::setMemoryManager((uintptr_t)globalMemMgr);
}

void dstm_p1_lib_shutdown()
{
	dstm_malloc_shutdown();
	if (globalCache != NULL) delete globalCache;
	if (globalMemMgr != NULL) delete globalMemMgr;
}
