cmake_minimum_required(VERSION 3.30)

project(
    Mocha
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(imgui)

add_executable(
    app
    src/animator.cpp
    src/engine.cpp
    src/keyframes.cpp
    src/main.cpp
    src/model.cpp
    src/shader.cpp
    src/skybox.cpp
    src/textures.cpp

    # Imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(app PRIVATE include)
target_include_directories(app PUBLIC ${imgui_SOURCE_DIR})
target_compile_options(app PRIVATE -Wall -Wextra -Werror)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG origin/main
    GIT_SHALLOW 1
)
set(BUILD_SHARED_LIBS OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
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
