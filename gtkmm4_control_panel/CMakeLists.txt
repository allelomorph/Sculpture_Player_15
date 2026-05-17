cmake_minimum_required(VERSION 3.16)

project(control_panel LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(Getcmake_utils)
include(PreventInSourceBuild)

add_executable(${PROJECT_NAME})
add_subdirectory(src)
target_link_libraries(${PROJECT_NAME} PRIVATE
  src
)

if(${PROJECT_NAME}_BUILD_DOCS AND PROJECT_IS_TOP_LEVEL)
  include(AddDoxygen)
  add_doxygen(${PROJECT_SOURCE_DIR}/src
    EXTRACT_ALL
    EXTRACT_PRIVATE
    EXTRACT_PRIV_VIRTUAL
    EXTRACT_PACKAGE
    EXTRACT_STATIC
    EXTRACT_ANON_NSPACES
    CALL_GRAPH
    CALLER_GRAPH
  )
endif()
