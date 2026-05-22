cmake_minimum_required(VERSION 3.21)

project(Playground
    VERSION 0.1.0
    DESCRIPTION "Qt host workspace for composing Camera, GraphicsEngine, and Gocator modules."
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets OpenGL Xml Concurrent)

set(PLAYGROUND_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(PLAYGROUND_MODULES_DIR "${PLAYGROUND_ROOT}/modules")

add_subdirectory("${PLAYGROUND_MODULES_DIR}/GraphicsEngine" "${CMAKE_BINARY_DIR}/modules/GraphicsEngine")
add_subdirectory("${PLAYGROUND_MODULES_DIR}/Camera/C++" "${CMAKE_BINARY_DIR}/modules/Camera")
add_subdirectory("${PLAYGROUND_MODULES_DIR}/Gocator" "${CMAKE_BINARY_DIR}/modules/Gocator")
add_subdirectory("${PLAYGROUND_MODULES_DIR}/Resources" "${CMAKE_BINARY_DIR}/modules/Resources")


qt_add_executable(Playground
    src/main.cpp
)


target_link_libraries(Playground
    PRIVATE
        GraphicsEngine::GraphicsEngine
        Camera
        gocator_core
        Resources::Resources
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::Xml
        Qt6::Concurrent
)

set_target_properties(Playground PROPERTIES
    BUILD_RPATH "${PLAYGROUND_MODULES_DIR}/Gocator/GoPxL-SDK/lib"
)
