cmake_minimum_required (VERSION 2.6)
project(vkEngine C CXX)

set(ENGINE_NAME ${PROJECT_NAME})
set(BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(LIB_DIR "${BIN_DIR}")

#message("O: ${CMAKE_BINARY_DIR} / ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(LIB_SYMBOL_DEBUG_DIR "${CMAKE_BINARY_DIR}/Debug")
set(LIB_SYMBOL_RELEASE_DIR "${CMAKE_BINARY_DIR}/Release")

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

set(ARCHITECTURE "")
message(${CMAKE_SYSTEM_NAME})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DVKE_WINDOWS)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DVKE_LINUX)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    add_definitions(-DVKE_ANDROID)
else()
    message(FATAL_ERROR "Unknown system")
endif()

if (ANDROID)
    add_definitions(-std=c++11)
    include_directories(android-windowing)
    add_subdirectory(android-windowing)
else()
    if (${CMAKE_GENERATOR} MATCHES "Visual Studio")
        set(CPP_FLAGS "/MP /W4 /fp:fast /GR- /GF")
        set(CPP_DEBUG_FLAGS "/MTd /EHsc /arch:IA32 /Qpar-")
        set(CPP_RELEASE_FLAGS "/MT /arch:AVX2 /Oi /Qpar")
    else()
        set(CPP_FLAGS "-fms-extensions -std=c++11 -Wall")
        set(CPP_DEBUG_FLAGS "")
        set(CPP_RELEASE_FLAGS "")
        add_definitions(-std=c++11)
    endif() # ${CMAKE_GENERATOR} MATCHES "Visual Studio"
endif()

#message("${CMAKE_GENERATOR} / ${CMAKE_CXX_COMPILER_ID} / ${ARCHITECTURE}")

add_definitions(-DVKE_API_EXPORT)

set(LIB_RELEASE_NAME "${PROJECT_NAME}${ARCHITECTURE}")
set(LIB_DEBUG_NAME "${LIB_RELEASE_NAME}_d")

set(CMAKE_C_FLAGS_RELEASE	         "${CMAKE_CXX_FLAGS_RELEASE} ${CPP_FLAGS} ${CPP_RELEASE_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG		         "${CMAKE_CXX_FLAGS_DEBUG} ${CPP_FLAGS}  ${CPP_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE	         "${CMAKE_CXX_FLAGS_RELEASE} ${CPP_FLAGS} ${CPP_RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG	         "${CMAKE_CXX_FLAGS_DEBUG} ${CPP_FLAGS}  ${CPP_DEBUG_FLAGS}")

message("Debug C++ flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message("Release C++ flags: ${CMAKE_CXX_FLAGS_RELEASE}")

if (CMAKE_BUILD_TYPE STREQUAL "debug")
    add_definitions(-D_DEBUG)
endif()

set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
message("Include dirs: ${INC_DIR}")
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET(INC_FILES ${INC_DIR}/*.h ${INC_DIR}/*.inl)
SET(SRC_FILES ${SRC_DIR}/*.h ${SRC_DIR}/*.cpp ${SRC_DIR}/*.inl)

file(GLOB_RECURSE INCLUDE_FILES ${INC_FILES})
file(GLOB_RECURSE SOURCE_FILES ${SRC_FILES})
file(GLOB_RECURSE ALL_FILES ${INC_FILES})

include_directories(${INC_DIR}/vke)
include_directories(${INC_DIR})
include_directories(src)
SUBDIRLIST(INCLUDE_SUBDIRS ${INC_DIR})
foreach(DIR ${INCLUDE_SUBDIRS})
    #include_directories("include/${DIR}")
endforeach()

add_library(${PROJECT_NAME} SHARED
    ${INCLUDE_FILES}
    ${SOURCE_FILES}
)

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${BIN_DIR}/post-build.bat)

#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}")
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${LIB_DEBUG_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${LIB_RELEASE_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${LIB_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES IMPORT_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${LIB_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES EXECUTABLE_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIB_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES IMPORT_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIB_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES EXECUTABLE_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})

foreach(f ${INCLUDE_FILES})
    # Get the path of the file relative to ${DIRECTORY},
    # then alter it (not compulsory)
    file(RELATIVE_PATH GRP ${INC_DIR} ${f})
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
    file(RELATIVE_PATH GRP ${SRC_DIR} ${f})
    set(GRP "src/${GRP}")
    # Extract the folder, ie remove the filename part
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" GRP ${GRP})
    # Source_group expects \\ (double antislash), not / (slash)
    string(REPLACE / \\ GRP ${GRP})
    source_group("${GRP}" FILES ${f})
endforeach()

if (ANDROID)
    target_link_libraries(triangle android_windowing)
endif()

# SAMPLES

set(SAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/samples")
SUBDIRLIST(SAMPLE_SUBDIRS ${SAMPLE_DIR})

foreach(DIR ${SAMPLE_SUBDIRS})
    message(${DIR})
    set(FULLPATH_DIR "${SAMPLE_DIR}/${DIR}")
    project(${DIR} C CXX)
    set(APP_RELEASE_NAME "${PROJECT_NAME}${ARCHITECTURE}")
    set(APP_DEBUG_NAME "${APP_RELEASE_NAME}_d")
    file(GLOB_RECURSE FILES ${FULLPATH_DIR}/*.h ${FULLPATH_DIR}/*.cpp)
    
    include_directories(${INC_DIR})
    link_directories(${LIB_DIR} ${BIN_DIR} ${LIB_SYMBOL_DEBUG_DIR} ${LIB_SYMBOL_RELEASE_DIR})
    
    add_executable(${PROJECT_NAME} ${FILES})
    add_dependencies(${PROJECT_NAME} vkEngine)
    message("${PROJECT_NAME}->${ENGINE_NAME}")
    
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME_DEBUG ${APP_DEBUG_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME_RELEASE ${APP_RELEASE_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})
    
    set(LINK_LIB optimized ${LIB_RELEASE_NAME} debug ${LIB_DEBUG_NAME})
    target_link_libraries(${PROJECT_NAME} ${LINK_LIB})
endforeach()