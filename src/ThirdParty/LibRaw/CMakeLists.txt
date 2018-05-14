cmake_minimum_required (VERSION 2.6)

project(libraw C CXX)

#file(GLOB_RECURSE LIBRAW_FILES "LibRaw/*.h" "LibRaw/*.cpp" "LibRaw/*.hpp" "LibRaw/*.c" "src/*.cpp")
#file(GLOB_RECURSE INTERNAL_FILES "internal/*.h" "internal/*.cpp")


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(libraw STATIC
    internal/defines.h
    internal/libraw_internal_funcs.h
    internal/var_defines.h
    libraw/libraw.h
    libraw/libraw_alloc.h
    libraw/libraw_const.h
    libraw/libraw_datastream.h
    libraw/libraw_internal.h
    libraw/libraw_types.h
    libraw/libraw_version.h
    internal/dcraw_common.cpp
    internal/dcraw_fileio.cpp
    internal/demosaic_packs.cpp
    src/libraw_c_api.cpp
    src/libraw_cxx.cpp
    src/libraw_datastream.cpp)

target_compile_definitions(libraw PRIVATE "LIBRAW_BUILDLIB")