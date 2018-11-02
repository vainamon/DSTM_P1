IF (NOT DEFINED LLVM_ROOT)
    SET (LLVM_ROOT "$ENV{LLVM_ROOT}")
ENDIF ()

IF (NOT EXISTS ${LLVM_ROOT}/include/llvm)
    MESSAGE(FATAL_ERROR "LLVM Root (${LLVM_ROOT}) is not a valid LLVM install!")
ENDIF()

SET (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_ROOT}/share/llvm/cmake")

INCLUDE (LLVM)
INCLUDE (LLVMConfig)
INCLUDE (AddLLVM)

ADD_DEFINITIONS (${LLVM_DEFINITIONS})

INCLUDE_DIRECTORIES(${LLVM_ROOT}/include)
LINK_DIRECTORIES(${LLVM_ROOT}/lib)

LLVM_MAP_COMPONENTS_TO_LIBRARIES(REQ_LLVM_LIBRARIES engine native)

SET (REQ_LLVM_LIBRARIES "${REQ_LLVM_LIBRARIES}" "LLVMBitReader" "LLVMBitWriter" "LLVMInstCombine" "LLVMLinker" "LLVMArchive" "LLVMipo" "dl")

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS")
