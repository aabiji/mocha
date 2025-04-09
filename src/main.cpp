#include <format>
#include <iostream>

#define IMGUI_USER_CONFIG "imgui_config.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include <glad.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "engine.h"

/*
TODO:
- transform meshes properly
- check if the mouse is hovering over a model
- Show side info based on the currently selected model
- upload lights and transformation matrices in a shader storage object
- improve the lighting (white shiny floor, lab like atmosphere)
- start researching human pose estimation
- We've got hdri assets from: https://polyhaven.com/
  Now just need to figure out how to actually use them
  More high quality 3d models can be found here: https://polyhaven.com/models
*/

struct App
{
    Engine engine;
    SDL_Window* window;
    SDL_GLContext context;
};

enum LogType {
    DEBUG = 32,
    WARN  = 33,
    ERROR = 31
};

void log(LogType type, std::string message)
{
    std::cout << "[\x1b[1;" << type << "m";
    std::cout << (type == DEBUG ? "DEBUG" : type == WARN ? "WARN" : "ERROR");
    std::cout << "\x1b[;39m] " << message << "\n";
    if (type == ERROR) std::abort(); // Exit with a backtrace
}

void debugCallback(
    unsigned int source, unsigned int type, unsigned int id,
    unsigned int severity, int length, const char *message, const void *param)
{
    (void)type;
    (void)id;
    (void)length;
    (void)param;

    std::string sources[] = {
        "opengl", "window system", "shader",
        "third party", "this application", "other source"
    };
    std::string sourceStr = sources[source - GL_DEBUG_SOURCE_API];

    LogType t = severity == GL_DEBUG_SEVERITY_HIGH
        ? ERROR
        : severity == GL_DEBUG_SEVERITY_MEDIUM ? WARN : DEBUG;
    log(t, std::format("From {} | {}", sourceStr, message));
}

SDL_AppResult SDL_AppInit(void** state, int argc, char** argv)
{
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    App* app = new App;

    app->window = SDL_CreateWindow("mocha", 900, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (app->window == nullptr)
        log(ERROR, SDL_GetError());

    app->context = SDL_GL_CreateContext(app->window);
    if (app->context == nullptr)
        log(ERROR, SDL_GetError());
    SDL_GL_MakeCurrent(app->window, app->context);

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
        log(ERROR, "Couldn't initialize glad!");
    glDebugMessageCallback(debugCallback, 0);

    try {
        app->engine.init(glm::vec4(230, 0, 900, 600));
    } catch (std::string msg) {
        log(ERROR, msg);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext(nullptr);

    ImGui_ImplSDL3_InitForOpenGL(app->window, app->context);
    ImGui_ImplOpenGL3_Init("#version 460");

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->AddFontFromFileTTF("../assets/Roboto-Regular.ttf", 15);
    ImGui::StyleColorsDark();

    state = (void**)&app;
    (void)state;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* state)
{
    App* app = (App*)state;
    unsigned int startMs = SDL_GetTicks();

    SDL_GL_MakeCurrent(app->window, app->context);
    app->engine.draw(double(SDL_GetTicks()) / 1000.0);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    app->engine.drawGUI();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(app->window);
    unsigned int endMs = SDL_GetTicks();

    float fps = std::floor(1000.0 / float(endMs - startMs));
    app->engine.setFPS(fps);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* state, SDL_Event* event)
{
    ImGui_ImplSDL3_ProcessEvent(event);

    App* app = (App*)state;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_WINDOW_RESIZED:
            app->engine.resizeViewport(event->window.data1, event->window.data2);
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_LEFT)
                app->engine.rotateCamera(true);
            if (event->key.key == SDLK_RIGHT)
                app->engine.rotateCamera(false);
            break;

        case SDL_EVENT_MOUSE_WHEEL:
        {
            int x = event->wheel.mouse_x;
            int y = event->wheel.mouse_y;
            if (app->engine.insideViewport(x, y))
                app->engine.zoomCamera(event->wheel.y);
            break;
        }

        case SDL_EVENT_MOUSE_MOTION:
            app->engine.mouseHover(event->motion.x, event->motion.y);
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* state, SDL_AppResult result)
{
    (void)result;

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    App* app = (App*)state;
    SDL_GL_DestroyContext(app->context);
    SDL_DestroyWindow(app->window);

    app->engine.cleanup();
    delete app;
}
