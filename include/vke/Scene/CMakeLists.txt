cmake_minimum_required(VERSION 3.5)

project(vkEngineScene C CXX)

set(INC_DIRS
	"${INCLUDE_DIR}/Scene/*.h"
	"${INCLUDE_DIR}/Scene/*.inl"
)
set(SRC_DIRS
	"${SOURCE_DIR}/Scene/*.h"
	"${SOURCE_DIR}/Scene/*.inl"
	"${SOURCE_DIR}/Scene/*.cpp"
)

set(SHADER_DIRS
	"${SOURCE_DIR}/Scene/*.hlsl"
	"${SOURCE_DIR}/Scene/*.hlsli"
)

file(GLOB_RECURSE INCLUDE_FILES ${INC_DIRS})
file(GLOB_RECURSE SOURCE_FILES ${SRC_DIRS})
file(GLOB_RECURSE SHADER_FILES ${SHADER_DIRS})
file(GLOB_RECURSE SCRIPT_FILES ${SCRIPT_DIRS})

add_library(vkEngineScene STATIC
	${INCLUDE_FILES}
	${SOURCE_FILES}
	${SHADER_FILES})

source_group(TREE "${INCLUDE_DIR}/Scene"
	PREFIX "include"
    FILES ${INCLUDE_FILES})
source_group(TREE "${SOURCE_DIR}/Scene"
	PREFIX "src"
    FILES ${SOURCE_FILES})
source_group(TREE "${SOURCE_DIR}/Scene"
    FILES ${SHADER_FILES})
	
add_dependencies(vkEngineScene vkEngineRenderSystem)

include(../../../LinkSettings.cmake NO_POLICY_SCOPE)

target_link_libraries(vkEngineScene debug vkEngineRenderSystem_d optimized vkEngineRenderSystem ${THIRD_PARTY_LIBS})
