CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT (DSTM_P1)

SET (CMAKE_VERBOSE_MAKEFILE ON)

SET (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${DSTM_P1_SOURCE_DIR}/../cmake")

# Intel compiler

#INCLUDE(Intel-toolchain)

# LLVM 

INCLUDE (LLVM-support)

# GASNet

IF (${GASNET_IBV})
    INCLUDE (GASNet-ibv-par)
ELSEIF (${GASNET_UDP})
    INCLUDE (GASNet-udp-par)
ELSEIF (${GASNET_SMP})
    INCLUDE (GASNet-smp-par)
ENDIF ()

# Boost

SET (BOOST_COMPONENTS thread system)

SET (Boost_USE_STATIC_LIBS ON)

find_package (Boost COMPONENTS ${BOOST_COMPONENTS})

IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
    link_directories(${Boost_LIBRARY_DIRS})
ELSEIF (Boost_FOUND)
    message(FATAL_ERROR "boost not installed!")
ENDIF (Boost_FOUND)

SET (Boost_LP "/sfsdata/lib")
# Boost static linkage
# SET (Boost_LIBRARIES ${Boost_LP}/libboost_system.a ${Boost_LP}/libboost_thread.a ${Boost_LP}/libboost_task.a ${Boost_LP}/libboost_fiber.a)
#SET (Boost_LIBRARIES ${Boost_LP}/libboost_system.a ${Boost_LP}/libboost_thread.a)

# TBB concurrent_hash_map

INCLUDE_DIRECTORIES ("tbb/include")
INCLUDE_DIRECTORIES ("tbb/src")
INCLUDE_DIRECTORIES ("tbb/src/rml/include")
ADD_SUBDIRECTORY (tbb)

# tanger

ADD_SUBDIRECTORY (tanger)

# gflags
INCLUDE (FindGflags)
SET (GFLAGS_LIB "${GFLAGS_LIBRARIES}")
include_directories (${GFLAGS_INCLUDE_DIRS}/include)
include_directories (${GFLAGS_INCLUDE_DIRS})

# logging

IF (LOGGING OR PROFILING)
    INCLUDE (FindGlog)
    SET (GLOG_LIB "${GLOG_LIBRARIES}")
    include_directories (${GLOG_INCLUDE_DIRS}/include)
    include_directories (${GLOG_INCLUDE_DIRS})

    IF(LOGGING)
        SET (OPT_DEFINES "${OPT_DEFINES} -DLOGGING")
    ENDIF()

    IF(PROFILING)
        SET (OPT_DEFINES "${OPT_DEFINES} -DPROFILING")
    ENDIF()

    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_DEFINES}")
    
ENDIF ()

# stats

IF (TRANSSTATS)

    SET (OPT_DEFINES "${OPT_DEFINES} -DTRANSACTIONS_STATS")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_DEFINES}")

ENDIF ()

# cache

IF (CACHE)

    SET (OPT_DEFINES "${OPT_DEFINES} -DCACHE")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_DEFINES}")

ENDIF ()

IF (VARIANT1)

    SET (OPT_DEFINES "${OPT_DEFINES} -DVARIANT1")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_DEFINES}")

ENDIF ()

IF (VARIANT2)

    SET (OPT_DEFINES "${OPT_DEFINES} -DVARIANT2")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_DEFINES}")

ENDIF ()

IF (VARIANT3)

    SET (OPT_DEFINES "${OPT_DEFINES} -DVARIANT3")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_DEFINES}")

ENDIF ()

# Application

file (GLOB DSTM_HEADERS
        "*.h")

file (GLOB DSTM_SOURCES
        "*.cpp")

#LINK_DIRECTORIES (/opt/hpmpi/lib/linux_amd64/)

ADD_EXECUTABLE(main ${DSTM_SOURCES} ${DSTM_HEADERS})

ADD_SUBDIRECTORY (dstm_pthread)

ADD_SUBDIRECTORY (dstm_malloc)

ADD_SUBDIRECTORY (dstm_lib_p1)

TARGET_LINK_LIBRARIES(main ${GASNET_LIBS} ${REQ_LLVM_LIBRARIES} ${Boost_LIBRARIES} ${GFLAGS_LIB} ${GLOG_LIB} tbb_chm tanger LLVMPthreadCreateReplacePass pthread)

INSTALL (TARGETS main DESTINATION ${CMAKE_INSTALL_PREFIX})

# Tests

ENABLE_TESTING()

ADD_SUBDIRECTORY (tests)
