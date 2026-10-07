set(RHI_NAME "D3D12")
set(RHI_LIBRARY_NAME "vkEngineRHI_${RHI_NAME}")

file(GLOB_RECURSE VKE_RHI_FILES CONFIGURE_DEPENDS
    "${INCLUDE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.h"
    "${INCLUDE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.inl"
    "${SOURCE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.h"
    "${SOURCE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.inl"
    "${SOURCE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.cpp")

add_library(${RHI_LIBRARY_NAME} STATIC ${VKE_RHI_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${VKE_RHI_FILES})
target_include_directories(${RHI_LIBRARY_NAME} PRIVATE "${INCLUDE_DIR}" "${SOURCE_DIR}")

# Lookout for D3D12 library. If not found then we need to add the Windows SDK Lib path to the search dirs.
# This way CMake can find D3D12.LIB required by the DirectX12 Agility SDK without additional manual steps.
# The Visual Studio generator runs without the x64 developer environment (no LIB variable), so a plain
# search can pick up the x86 import library. Point it at the x64 SDK dir of the selected SDK version.
set(_D3D12_LIB_HINTS)
if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    get_filename_component(_WIN_KITS_HINT_ROOT
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]"
        ABSOLUTE)
    set(_D3D12_LIB_HINTS "${_WIN_KITS_HINT_ROOT}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/x64")
endif()
find_library(D3D12_LIB NAMES d3d12 HINTS ${_D3D12_LIB_HINTS})

if("${D3D12_LIB}" STREQUAL "D3D12_LIB-NOTFOUND")
    # Resolve the Windows Kits 10 root. The Visual Studio generator provides the SDK version
    # (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION) but not the root, and CMAKE_WINDOWS_KITS_10_DIR
    # is often empty. The SDK registers its root under the 32-bit WOW6432Node registry key, so
    # read that (a 64-bit CMake reading the plain key returns the wrong location).
    set(_WIN_KITS_ROOT "${CMAKE_WINDOWS_KITS_10_DIR}")
    if(NOT _WIN_KITS_ROOT)
        get_filename_component(_WIN_KITS_ROOT
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]"
            ABSOLUTE)
    endif()

    set(_WIN_SDK_INCLUDE_PATH "${_WIN_KITS_ROOT}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/x64")
    message(STATUS "[WINSDK]: Adding Lib dir to library search path: ${_WIN_SDK_INCLUDE_PATH}")

    # Add _WIN_SDK_INCLUDE_PATH to the library search path
    list(APPEND CMAKE_LIBRARY_PATH "${_WIN_SDK_INCLUDE_PATH}")
endif()

find_package(directx-headers CONFIG REQUIRED)
find_package(directx12-agility CONFIG REQUIRED)
find_package(winpixevent CONFIG REQUIRED)

set(LINK_D3D12_LIBS dxgi dxguid Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility Microsoft::WinPixEventRuntime)

# ---------------------------------------------------------------------------
# Agility SDK runtime deployment (generator agnostic: single and multi-config)
# ---------------------------------------------------------------------------
set(_AGILITY_BIN_DIR     "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
set(_AGILITY_DBG_BIN_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/bin")

# OUTPUT_DIR already contains $<CONFIG>, so this follows the per-config layout.
set(_AGILITY_DST_DIR "${OUTPUT_DIR}/D3D12")

# D3D12Core.dll exists in both bin/ and debug/bin.
# d3d12SDKLayers.dll only ships in debug/bin, so it is only deployed for Debug.
# The whole list is a single genex so no empty arguments are ever produced.
set(_AGILITY_SRC_DLLS
    "$<IF:$<CONFIG:Debug>,${_AGILITY_DBG_BIN_DIR}/D3D12Core.dll$<SEMICOLON>${_AGILITY_DBG_BIN_DIR}/d3d12SDKLayers.dll,${_AGILITY_BIN_DIR}/D3D12Core.dll>")

set(_AGILITY_STAMP "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/agility_deploy.stamp")

add_custom_command(
    OUTPUT "${_AGILITY_DST_DIR}/D3D12Core.dll" "${_AGILITY_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_AGILITY_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_AGILITY_SRC_DLLS} "${_AGILITY_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${_AGILITY_STAMP}"
    DEPENDS ${_AGILITY_SRC_DLLS}
    COMMENT "Deploying D3D12 Agility SDK runtime to ${_AGILITY_DST_DIR}"
    COMMAND_EXPAND_LISTS
    VERBATIM)

add_custom_target(${RHI_LIBRARY_NAME}_Deploy ALL
    DEPENDS "${_AGILITY_DST_DIR}/D3D12Core.dll" "${_AGILITY_STAMP}")
set_target_properties(${RHI_LIBRARY_NAME}_Deploy PROPERTIES FOLDER "vkEngine/RHI")
add_dependencies(${RHI_LIBRARY_NAME} ${RHI_LIBRARY_NAME}_Deploy)

install(FILES ${_AGILITY_SRC_DLLS} DESTINATION bin/D3D12)

target_link_libraries(${RHI_LIBRARY_NAME} PRIVATE vkEngineCore ${LINK_D3D12_LIBS} )
target_compile_definitions(${RHI_LIBRARY_NAME} PRIVATE ${PREPROCESSOR_DEFINITIONS} VKE_API_EXPORT)
set_target_properties(${RHI_LIBRARY_NAME} PROPERTIES
    FOLDER "vkEngine/RHI"
    POSITION_INDEPENDENT_CODE ON)
