include(FindPackageHandleStandardArgs)

set(GFLAGS_ROOT_DIR "" CACHE PATH "Folder contains Google gflags")

if(WIN32)
    find_path(GFLAGS_INCLUDE_DIR gflags/gflags.h
        PATHS ${GFLAGS_ROOT_DIR}/src/windows)
else()
    find_path(GFLAGS_INCLUDE_DIR gflags/gflags.h
        PATHS ${GFLAGS_ROOT_DIR}/include)
endif()

if(MSVC)
    find_library(GFLAGS_LIBRARY_RELEASE libgflags_static
        PATHS ${GFLAGS_ROOT_DIR}
        PATH_SUFFIXES Release)

    find_library(GFLAGS_LIBRARY_DEBUG libgflags_static
        PATHS ${GFLAGS_ROOT_DIR}
        PATH_SUFFIXES Debug)

    set(GFLAGS_LIBRARY optimized ${GFLAGS_LIBRARY_RELEASE} debug ${GFLAGS_LIBRARY_DEBUG})
else()
    find_library(GFLAGS_LIBRARY libgflags.a gflags
        PATHS ${GFLAGS_ROOT_DIR}
        PATH_SUFFIXES
            lib
            lib64)
endif()

find_package_handle_standard_args(GFLAGS DEFAULT_MSG
    GFLAGS_INCLUDE_DIR GFLAGS_LIBRARY)

if(GFLAGS_FOUND)
    set(GFLAGS_INCLUDE_DIRS ${GFLAGS_INCLUDE_DIR})
    set(GFLAGS_LIBRARIES ${GFLAGS_LIBRARY})
endif()
