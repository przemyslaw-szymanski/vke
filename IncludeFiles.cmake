cmake_minimum_required(VERSION 2.6)

file(GLOB_RECURSE INCLUDE_FILES ${INC_DIRS})
file(GLOB_RECURSE SOURCE_FILES ${SRC_DIRS})

foreach(f ${INCLUDE_FILES})
    # Get the path of the file relative to ${DIRECTORY},
    # then alter it (not compulsory)
    file(RELATIVE_PATH GRP ${INCLUDE_DIR} ${f})
    set(GRP "include/${GRP}")
    # Extract the folder, ie remove the filename part
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" GRP ${GRP})
    # Source_group expects \\ (double antislash), not / (slash)
    string(REPLACE / \\ GRP ${GRP})
    source_group("${GRP}" FILES ${f})
endforeach()

foreach(f ${SOURCE_FILES})
    # Get the path of the file relative to ${DIRECTORY},
    # then alter it (not compulsory)
    file(RELATIVE_PATH GRP ${SOURCE_DIR} ${f})
    set(GRP "src/${GRP}")
    # Extract the folder, ie remove the filename part
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" GRP ${GRP})
    # Source_group expects \\ (double antislash), not / (slash)
    string(REPLACE / \\ GRP ${GRP})
    source_group("${GRP}" FILES ${f})
endforeach()

set(THIRD_PARTY_INC "")

if(VKE_USE_DEVIL)
     set(THIRD_PARTY_INC "${THIRD_PARTY_INC}" "${SOURCE_DIR}/ThirdParty/DevIL/DevIL/include/")
endif()
if(VKE_USE_DIRECTXTEX)
     set(THIRD_PARTY_INC "${THIRD_PARTY_INC}" "${SOURCE_DIR}/ThirdParty/DirectXTex")
endif()
if(VKE_USE_DIRECTX_SHADER_COMPILER)
    set(THIRD_PARTY_INC "${THIRD_PARTY_INC}" "${SOURCE_DIR}/ThirdParty/dxc/include")
endif()

if(VKE_USE_GAINPUT)
    set(THIRD_PARTY_INC ${THIRD_PARTY_INC} "${THIRD_PARTY_DIR}/gainput-1.0.0/lib/include")
endif()

include_directories("${INCLUDE_DIR}" "${SOURCE_DIR}" "${THIRD_PARTY_INC}")