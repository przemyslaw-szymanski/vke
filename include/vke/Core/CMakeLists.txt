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

# Compile only the selected input system API implementation
if(NOT VKE_INPUT_SYSTEM_WINAPI)
	list(FILTER SOURCE_FILES EXCLUDE REGEX "/Core/Input/WinAPI/")
endif()

if(NOT VKE_INPUT_SYSTEM_XI2)
	list(FILTER SOURCE_FILES EXCLUDE REGEX "/Core/Input/XI2/")
endif()

# Compile only the selected window system API implementation
if(NOT VKE_WINDOW_SYSTEM_WINAPI)
	list(FILTER SOURCE_FILES EXCLUDE REGEX "/Core/Platform/Window/WinAPI/")
endif()

if(NOT VKE_WINDOW_SYSTEM_XCB)
	list(FILTER SOURCE_FILES EXCLUDE REGEX "/Core/Platform/Window/XCB/")
endif()

add_library(vkEngineCore STATIC
	${INCLUDE_FILES}
	${SOURCE_FILES})

target_include_directories(vkEngineCore PRIVATE ${CORE_INC_DIR})
target_link_libraries(vkEngineCore PUBLIC ${THIRD_PARTY_LIBS})

# XCB is a system library: link it dynamically so the process shares one libxcb with the Vulkan WSI driver.
if(VKE_WINDOW_SYSTEM_XCB)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
	target_link_libraries(vkEngineCore PUBLIC PkgConfig::XCB)
endif()

if(VKE_INPUT_SYSTEM_XI2)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(XCB_XINPUT IMPORTED_TARGET xcb-xinput)
	if(NOT XCB_XINPUT_FOUND)
		message(FATAL_ERROR "xcb-xinput not found. Install it with: sudo apt install libxcb-xinput-dev")
	endif()
	target_link_libraries(vkEngineCore PUBLIC PkgConfig::XCB_XINPUT)
endif()

include(../../../LinkSettings.cmake NO_POLICY_SCOPE)
