INCLUDE (CMakeForceCompiler)

SET (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

SET (CMAKE_STATIC_LIBRARY_LINK_C_FLAGS "")
SET (CMAKE_STATIC_LIBRARY_LINK_CXX_FLAGS "")

SET (CMAKE_SKIP_RPATH true)

CMAKE_FORCE_C_COMPILER (llvm-gcc DTMC-gcc)
CMAKE_FORCE_CXX_COMPILER (llvm-g++ DTMC-g++)

remove_definitions(-std=c++0x)
remove_definitions(-ip)
remove_definitions(-gcc)
remove_definitions(-static)
#remove_definitions(-ipo_separate)
remove_definitions(-xhost)
remove_definitions(-inline-level=2)

SET (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -c --emit-llvm")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -c --emit-llvm")

SET (CMAKE_RANLIB "llvm-ranlib" CACHE INTERNAL STRING)
SET (CMAKE_AR "llvm-ar" CACHE INTERNAL STRING)

SET (CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> cr <TARGET> <LINK_FLAGS> <OBJECTS>")
#-DCMAKE_BUILD_TYPE=Release
SET (CMAKE_LINKER "llvm-ld" CACHE INTERNAL STRING)

SET (CMAKE_C_LINK_EXECUTABLE "llvm-ld <OBJECTS> -o <TARGET> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")
SET (CMAKE_CXX_LINK_EXECUTABLE "llvm-ld <OBJECTS> -o <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")

SET (CMAKE_FIND_ROOT_PATH /usr/bin)
SET (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
