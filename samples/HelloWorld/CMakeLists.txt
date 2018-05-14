cmake_minimum_required (VERSION 2.6)
get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId} C CXX)

set(APP_RELEASE_NAME "${PROJECT_NAME}${ARCHITECTURE}")
set(APP_DEBUG_NAME "${APP_RELEASE_NAME}_d")
file(GLOB_RECURSE FILES *.h *.cpp)

include_directories(${INC_DIR})
link_directories(${LIB_DIR} ${BIN_DIR} ${LIB_SYMBOL_DEBUG_DIR} ${LIB_SYMBOL_RELEASE_DIR})

message("add_executable: ${PROJECT_NAME} ${FILES}")
add_executable(${PROJECT_NAME} ${FILES})
add_dependencies(${PROJECT_NAME} vkEngine)
message("${PROJECT_NAME}->${ENGINE_NAME}")

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME_DEBUG ${APP_DEBUG_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME_RELEASE ${APP_RELEASE_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${BIN_DIR}")

set(LINK_LIB optimized ${LIB_RELEASE_NAME} debug ${LIB_DEBUG_NAME})
target_link_libraries(${PROJECT_NAME} ${LINK_LIB})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Samples")
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)