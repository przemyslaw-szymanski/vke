cmake_minimum_required(VERSION 3.5)

project(vkEngineScene C CXX)

function(AddShader fileName shaderType entryPoint)
    message(STATUS "Adding shader: ${fileName} of type ${shaderType} with entry point ${entryPoint}")
	set_source_files_properties( ${SOURCE_DIR}/Scene/Assets/Shaders/${fileName} PROPERTIES
		VS_SHADER_MODEL "6.6" 
		VS_SHADER_TYPE "${shaderType}"
		VS_SHADER_ENTRYPOINT "${entryPoint}"
		VS_SHADER_FLAGS "-enable-16bit-types")
endfunction()

function(add_shader_copy_postbuild TARGET SRC_DIR DST_DIR)
    file(GLOB_RECURSE SHADER_FILES
        "${SRC_DIR}/*.hlsl"
        "${SRC_DIR}/*.h"
        "${SRC_DIR}/*.hlsli"
        "${SRC_DIR}/*"
    )

    set(COPY_COMMANDS "")
    foreach(SHADER_FILE IN LISTS SHADER_FILES)
        file(RELATIVE_PATH REL_PATH "${SRC_DIR}" "${SHADER_FILE}")
        set(DST_FILE "${DST_DIR}/${REL_PATH}")

        # Ensure destination directory exists at build time
        get_filename_component(DST_PATH "${DST_FILE}" DIRECTORY)
        list(APPEND COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_PATH}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_FILE}" "${DST_FILE}"
        )
    endforeach()

    message(STATUS "Setting up post-build copy commands for target ${COPY_COMMANDS}")

    add_custom_command(TARGET ${TARGET} POST_BUILD
        ${COPY_COMMANDS}
        COMMENT "Copying shaders and headers to ${DST_DIR} (preserving hierarchy)"
    )
endfunction()

function(get_shader_type_by_filename FILENAME OUT_VAR)
    if(FILENAME MATCHES "\\.cs\\.hlsl$")
        set(${OUT_VAR} "Compute" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.vs\\.hlsl$")
        set(${OUT_VAR} "Vertex" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.ps\\.hlsl$")
        set(${OUT_VAR} "Pixel" PARENT_SCOPE)
    elseif(FILENAME MATCHES "\\.ms\\.hlsl$")
        set(${OUT_VAR} "Mesh" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "Unknown" PARENT_SCOPE)
    endif()
endfunction()

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

target_include_directories(${PROJECT_NAME}
	PRIVATE
	${INCLUDE_DIR}
	${SOURCE_DIR} ) 

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

add_shader_copy_postbuild(vkEngineScene
    "${SOURCE_DIR}/Scene/Assets"
    "${OUTPUT_DIR}/data/Assets"
)
target_link_libraries(vkEngineScene debug vkEngineRenderSystem_d optimized vkEngineRenderSystem ${THIRD_PARTY_LIBS})
