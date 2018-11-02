IF (UNIX)
  SET (LLVM_ON_UNIX 1)
ENDIF(UNIX)

INCLUDE (LLVM-toolchain)

SET (DSTM_LIB_P1_SOURCES dstm_p1-ls.cpp gasnet_mem_mgr.cpp dstm_p1.cpp dstm_transaction.cpp dstm_locks_cache.cpp dstm_transaction_impl.h memory_manager.h dstm_timestamps.h)

ADD_LLVM_EXECUTABLE (dstm_lib_p1 ${DSTM_LIB_P1_SOURCES})

INSTALL (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/dstm_lib_p1.bc DESTINATION ${CMAKE_INSTALL_PREFIX})
