cmake_minimum_required(VERSION 3.10...3.31)

project(vkEngineCore C CXX)

set(INC_DIRS
	"${INCLUDE_DIR}/Core/*.h"
	"${INCLUDE_DIR}/Core/*.inl"
)
set(SRC_DIRS
	"${SOURCE_DIR}/Core/*.h"
	"${SOURCE_DIR}/Core/*.inl"
	"${SOURCE_DIR}/Core/*.cpp"
)

include(../../../IncludeFiles.cmake NO_POLICY_SCOPE)

add_library(vkEngineCore STATIC
	${INCLUDE_FILES}
	${SOURCE_FILES})

target_link_libraries(vkEngineCore PUBLIC ${THIRD_PARTY_LIBS})
	
include(../../../LinkSettings.cmake NO_POLICY_SCOPE)
