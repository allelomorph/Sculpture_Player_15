
cmake_minimum_required(VERSION 3.16)

project(Gui_With_Player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED COMPONENTS CXX)

# Sources (make sure engine_stub.cpp actually exists or remove it)
set(SOURCE_FILES
    main.cpp
    kv_io.cpp
    function_misc.cpp
    fs_utils.cpp
    #player_class_UMat.cpp
    player_class_Mat.cpp    
    image_processor.cpp
    measure2.cpp
    sculpture.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Includes (scoped)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

# Compile opts: add O3 only for Release
target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Release>:-O3>
)

# Link libraries (scoped)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${OpenCV_LIBS}
        OpenMP::OpenMP_CXX
)

# FTDI (portable lookup)
find_library(FTD2XX_LIB ftd2xx PATHS /usr/local/lib /usr/lib /opt/homebrew/lib)
if(FTD2XX_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FTD2XX_LIB})
else()
    message(WARNING "libftd2xx not found; continuing without it")
endif()

# (Optional) show OpenMP details
get_target_property(_omp_intf OpenMP::OpenMP_CXX INTERFACE_LINK_LIBRARIES)
message(STATUS "OpenMP CXX target link interface: ${_omp_intf}")

# (Optional) convenience targets; safer to configure in a build dir
add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR} -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Debug
    COMMENT "Configure+Build Debug"
)
add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR} -DCMAKE_BUILD_TYPE=Release
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release
    COMMENT "Configure+Build Release"
)
