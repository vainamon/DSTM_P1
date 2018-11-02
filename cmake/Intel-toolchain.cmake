INCLUDE (CMakeForceCompiler)

CMAKE_FORCE_C_COMPILER (icc "Intel C compiler")
CMAKE_FORCE_CXX_COMPILER (icpc "Intel C++ compiler")

add_definitions(-std=c++0x)
add_definitions(-gcc)
add_definitions(-static)
add_definitions(-ip)
#add_definitions(-ipo_separate)
add_definitions(-xhost)
add_definitions(-inline-level=2)