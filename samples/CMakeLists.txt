cmake_minimum_required(VERSION 3.10...3.31)

project(Samples)

include(../CompileOptions.cmake NO_POLICY_SCOPE)
include(../CompilerSettings.cmake NO_POLICY_SCOPE)

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(INC_DIR "${CMAKE_SOURCE_DIR}/include")

add_subdirectory(HelloWorld)
add_subdirectory(Triangle)
add_subdirectory(MultiWindow)
add_subdirectory(BasicBackgroundLoading)
add_subdirectory(SimpleRenderTarget)
add_subdirectory(SimpleScene)
add_subdirectory(Terrain)
add_subdirectory(MeshShaders)