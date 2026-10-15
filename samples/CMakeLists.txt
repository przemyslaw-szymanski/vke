cmake_minimum_required(VERSION 3.10...3.31)

project(Samples)

include(../CompileOptions.cmake NO_POLICY_SCOPE)
include(../CompilerSettings.cmake NO_POLICY_SCOPE)

#set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
#set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(INC_DIR "${CMAKE_SOURCE_DIR}/include")

add_subdirectory(HelloWorld)
add_subdirectory(Triangle)
# Out of date with the engine API; excluded from the default build until fixed (still buildable explicitly).
add_subdirectory(MultiWindow EXCLUDE_FROM_ALL)
add_subdirectory(BasicBackgroundLoading EXCLUDE_FROM_ALL)
add_subdirectory(SimpleRenderTarget EXCLUDE_FROM_ALL)
add_subdirectory(SimpleScene EXCLUDE_FROM_ALL)
add_subdirectory(Terrain)
add_subdirectory(MeshShaders)