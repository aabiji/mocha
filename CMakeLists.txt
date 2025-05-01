cmake_minimum_required(VERSION 3.30)

project(
    Mocha
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_LINKER_TYPE MOLD)
set(CMAKE_BUILD_PARALLEL_LEVEL 4 FORCE)

add_executable(
    app
    src/main.cpp
)
target_compile_options(app PRIVATE -Wall -Wextra)

find_package(OpenCV REQUIRED)
include_directories(app ${OpenCV_INCLUDE_DIRS})
target_link_libraries(app ${OpenCV_LIBS})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)
target_link_libraries(app PkgConfig::LIBAV)

find_library(ONNX_RUNTIME_LIB onnxruntime)
find_path(ONNX_RUNTIME_INCLUDE onnxruntime_cxx_api.h PATH_SUFFIXES onnxruntime)
target_include_directories(app PRIVATE ${ONNX_RUNTIME_INCLUDE})
target_link_libraries(app ${ONNX_RUNTIME_LIB})
