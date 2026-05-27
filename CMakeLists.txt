cmake_minimum_required(VERSION 3.21)

project(Playground
    VERSION 0.1.0
    DESCRIPTION "Qt host workspace for composing Camera, GraphicsEngine, and Gocator modules."
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(PLAYGROUND_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(PLAYGROUND_MODULES_DIR "${PLAYGROUND_ROOT}/modules")

function(playground_require_module_cmake module_path)
    set(module_cmake "${PLAYGROUND_MODULES_DIR}/${module_path}/CMakeLists.txt")
    if(NOT EXISTS "${module_cmake}")
        message(FATAL_ERROR
            "Required module '${module_path}' is not checked out at '${module_cmake}'. "
            "Run 'git submodule sync --recursive' and "
            "'git submodule update --init --recursive' from the Playground root."
        )
    endif()
endfunction()

playground_require_module_cmake("GraphicsEngine")
playground_require_module_cmake("Camera/C++")
playground_require_module_cmake("Gocator/C++")
playground_require_module_cmake("Resources")

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets OpenGL Xml Concurrent)

add_subdirectory("${PLAYGROUND_MODULES_DIR}/GraphicsEngine" "${CMAKE_BINARY_DIR}/modules/GraphicsEngine")
add_subdirectory("${PLAYGROUND_MODULES_DIR}/Camera/C++" "${CMAKE_BINARY_DIR}/modules/Camera")
add_subdirectory("${PLAYGROUND_MODULES_DIR}/Gocator/C++" "${CMAKE_BINARY_DIR}/modules/Gocator")
add_subdirectory("${PLAYGROUND_MODULES_DIR}/Resources" "${CMAKE_BINARY_DIR}/modules/Resources")


qt_add_executable(Playground
    src/main.cpp
    src/MainWindow.cpp
    src/DeviceSession.cpp
    src/UI/QProcessingWidget.cpp
    src/UI/QStaticImageControlWidget.cpp
    src/Utility/LogManager.cpp
    src/Utility/ParameterParser.h
    src/Utility/ParameterParser.cpp
    src/Controller/AbstractImagingController.h
    src/Controller/GraphicsEngineSink.h
    src/Controller/GraphicsEngineSink.cpp
    src/Controller/CameraImagingController.h
    src/Controller/CameraImagingController.cpp
    src/Controller/GocatorImagingController.h
    src/Controller/GocatorImagingController.cpp
    src/Controller/StaticImageImagingController.h
    src/Controller/StaticImageImagingController.cpp
    src/Pipeline/ProcessingFrame.h
    src/Pipeline/ProcessingPipeline.h
    src/Pipeline/ProcessingPipeline.cpp
    src/Pipeline/ProcessingRegistry.h
    src/Pipeline/ProcessingRegistry.cpp
    src/Pipeline/DynamicLibraryLoader.h
    src/Pipeline/DynamicLibraryLoader.cpp
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

target_include_directories(Playground PRIVATE src)

set_target_properties(Playground PROPERTIES
    BUILD_RPATH "${PLAYGROUND_MODULES_DIR}/Gocator/GoPxL-SDK/lib"
)

if(UNIX AND NOT APPLE)
    # VTK requires Qt OpenGL indirectly; retain the explicitly linked Qt target in DT_NEEDED.
    target_link_options(Playground PRIVATE "-Wl,--no-as-needed")
endif()

target_compile_definitions(Playground PRIVATE
    PLAYGROUND_SRC_DIR="${CMAKE_CURRENT_LIST_DIR}/src"
)

if(POLICY CMP0146)
    # Current OpenCV config still probes CUDA through CMake's legacy module.
    cmake_policy(SET CMP0146 OLD)
endif()
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "[Playground] OpenCV found. Enabling live image processing pipeline.")
    target_compile_definitions(Playground PRIVATE HAS_OPENCV)

    get_filename_component(OPENCV_LIB_DIR "${OpenCV_DIR}/../.." ABSOLUTE)

    target_compile_definitions(Playground PRIVATE
        OPENCV_INCLUDE_DIR="${OpenCV_INCLUDE_DIRS}"
        OPENCV_LIB_DIR="${OPENCV_LIB_DIR}"
    )

    target_include_directories(Playground PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(Playground PRIVATE ${OpenCV_LIBRARIES})
else()
    message(WARNING "[Playground] OpenCV NOT found. Live image processing pipeline will be disabled.")
endif()
