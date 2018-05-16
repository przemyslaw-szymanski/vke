cmake_minimum_required(VERSION 2.6)

project(Samples)

include(../CompileOptions.cmake NO_POLICY_SCOPE)
include(../CompileSettings.cmake NO_POLICY_SCOPE)

set(OUTPUT_DIR "../../../bin")
set(LIB_DIR "../../lib")
set(INC_DIR "../../include")

add_subdirectory(HelloWorld)