cmake_minimum_required(VERSION 3.30)

project(
    Mocha
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LINKER_TYPE MOLD)

if(NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
    set(CMAKE_BUILD_PARALLEL_LEVEL
        16
        CACHE STRING "Default parallel build level" FORCE
    )
endif()

add_executable(
    app
    src/main.cpp
)
target_compile_options(app PRIVATE -Wall -Wextra)

find_package(OpenCV REQUIRED)
include_directories(app ${OpenCV_INCLUDE_DIRS})
target_link_libraries(app ${OpenCV_LIBS})

# TODO: tensorflow-lite has been rebranded as LiteRT
#       will need to migrate to the LiteRT repository
#       when the C++ SDK gets ported for LiteRT
include(FetchContent)
FetchContent_Declare(
    tensorflow
    GIT_REPOSITORY https://github.com/tensorflow/tensorflow
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(tensorflow)
add_subdirectory(
    "${tensorflow_SOURCE_DIR}/tensorflow/lite"
    "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite" EXCLUDE_FROM_ALL)

target_link_libraries(app tensorflow-lite)
