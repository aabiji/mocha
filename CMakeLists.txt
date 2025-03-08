cmake_minimum_required(VERSION 3.30)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

project(
    OpenGLTemplate
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)

add_executable(
    app 
    src/main.cc
    src/shader.cc
    src/camera.cc
)
target_compile_options(app PRIVATE -Wall -Wextra -Werror)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)
target_link_libraries(app glfw)

# GLAD
add_library(glad libs/glad/src/glad.c)
target_include_directories(glad PUBLIC libs/glad/include)
target_link_libraries(app glad)

# stb libraries
target_include_directories(app PUBLIC libs/stb)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(glm)
target_link_libraries(app glm::glm)

# Assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_SHALLOW 1
)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_TESTS OFF)
FetchContent_MakeAvailable(assimp)
target_link_libraries(app assimp::assimp)
