cmake_minimum_required (VERSION 2.6)
get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId} C CXX)

cmake_policy(SET CMP0015 NEW)

set(APP_RELEASE_NAME "${PROJECT_NAME}${ARCHITECTURE}")
set(APP_DEBUG_NAME "${APP_RELEASE_NAME}_d")
file(GLOB_RECURSE FILES *.h *.cpp)

include_directories(${INC_DIR})
link_directories(${LIB_DIR} ${OUTPUT_DIR})

add_executable(${PROJECT_NAME} ${FILES})
add_dependencies(${PROJECT_NAME} vkEngine)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME_DEBUG ${APP_DEBUG_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME_RELEASE ${APP_RELEASE_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}")

set(LINK_LIB optimized vkEngine debug vkEngine_d)
target_link_libraries(${PROJECT_NAME} ${LINK_LIB})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Samples")
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PREPROCESSOR_DEFINITIONS})