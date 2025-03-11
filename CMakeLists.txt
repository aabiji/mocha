cmake_minimum_required(VERSION 3.30)

project(
  Mocha
  VERSION 1.0
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)

add_executable(
  app
  src/main.cpp
  src/shader.cpp
  src/camera.cpp
)
target_include_directories(app PRIVATE include)
target_compile_options(app PRIVATE -Wall -Wextra -Werror)

include(FetchContent)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG origin/main
  GIT_SHALLOW 1
)
FetchContent_MakeAvailable(SDL3)
target_link_libraries(app SDL3::SDL3-static)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_SHALLOW 1
)
FetchContent_MakeAvailable(glm)
target_link_libraries(app glm)

FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_SHALLOW 1
)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_TESTS OFF)
FetchContent_MakeAvailable(assimp)
target_link_libraries(app assimp)