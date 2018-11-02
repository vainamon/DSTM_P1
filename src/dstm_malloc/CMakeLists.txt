IF (UNIX)
  SET (LLVM_ON_UNIX 1)
ENDIF(UNIX)

INCLUDE (LLVM-toolchain)

ADD_LIBRARY (dstm_malloc dstm_malloc.cpp buddy.cpp)

INSTALL (TARGETS dstm_malloc DESTINATION ${CMAKE_INSTALL_PREFIX})
