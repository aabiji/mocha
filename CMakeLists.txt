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
endif ()

include(FetchContent)

add_executable(
    app
    src/main.cpp
)

target_compile_options(app PRIVATE -Wall -Wextra -Werror)
