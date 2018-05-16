cmake_minimum_required(VERSION 2.6)

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
include(../../../IncludeFiles.cmake NO_POLICY_SCOPE)

add_library(vkEngineScene STATIC
	${INCLUDE_FILES}
	${SOURCE_FILES})
	
add_dependencies(vkEngineScene vkEngineRenderSystem)

include(../../../LinkSettings.cmake NO_POLICY_SCOPE)

target_link_libraries(vkEngineScene debug vkEngineRenderSystem_d optimized vkEngineRenderSystem)
