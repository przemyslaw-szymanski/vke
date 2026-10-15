cmake_minimum_required(VERSION 3.10...3.31)

project(vkEngineCore C CXX)

set(CORE_INC_DIR "${INCLUDE_DIR}/Core")

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

target_include_directories(vkEngineCore PRIVATE ${CORE_INC_DIR})
target_link_libraries(vkEngineCore PUBLIC ${THIRD_PARTY_LIBS})

# XCB is a system library: link it dynamically so the process shares one libxcb with the Vulkan WSI driver.
if(VKE_LINUX)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
	target_link_libraries(vkEngineCore PUBLIC PkgConfig::XCB)
endif()

include(../../../LinkSettings.cmake NO_POLICY_SCOPE)
