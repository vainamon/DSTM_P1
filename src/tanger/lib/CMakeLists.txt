SET (TANGER_SOURCES tanger.cpp transformations.cpp support.cpp interface.cpp)

ADD_LLVM_LIBRARY (tanger ${TANGER_SOURCES})