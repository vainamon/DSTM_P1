SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PTHREAD -D__TBB_BUILD -DTBB_USE_ASSERT -DTBB_USE_THREADING_TOOLS")
SET (TBB_SOURCES concurrent_hash_map.cpp spin_rw_mutex.cpp cache_aligned_allocator.cpp dynamic_link.cpp tbb_misc.cpp itt_notify.cpp tbb_main.cpp governor.cpp market.cpp arena.cpp scheduler.cpp observer_proxy.cpp task_group_context.cpp tbb_misc_ex.cpp ../rml/client/rml_tbb.cpp private_server.cpp task.cpp)
SET (TBB_SOURCES ${TBB_SOURCES} spin_mutex.cpp)
ADD_LIBRARY (tbb_chm ${TBB_SOURCES})