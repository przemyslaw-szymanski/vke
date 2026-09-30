cmake_minimum_required(VERSION 3.10...3.31)

file(GLOB_RECURSE INCLUDE_FILES ${INC_DIRS})
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Header Files" FILES ${INCLUDE_FILES})

file(GLOB_RECURSE SOURCE_FILES ${SRC_DIRS})
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCE_FILES})

include_directories("${INCLUDE_DIR}" "${SOURCE_DIR}")