#define IMGUI_USER_CONFIG "imgui_config.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#include <glad.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "engine.h"
#include "log.h"

/*
TODO -- we have a working demo, we now need to integrate it into the engine:(for the texture, model and skybox)
- Read: https://en.wikipedia.org/wiki/3D_pose_estimation
    - Maybe pull in ffmpeg to read keypoints from a video, so we can "replay" it,
      instead of relying on the webcam to test the skeletal animation mapping
    - Figure out how to map the changes in keypoints into bone transforms
- When we figure out what realistic human model we'll use, we can write a small
  script to download all the necessary assets automatically
 */

static void debugCallback(
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
    log(t, "From " + sourceStr + " | " + message);
}

struct App
{
    Engine engine;
    SDL_Window* window;

    SDL_GLContext context;
    bool shouldRender;

    SDL_Camera* camera;
    bool canReadCamera;
};

SDL_AppResult SDL_AppInit(void** state, int argc, char** argv)
{
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_CAMERA);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    App* app = new App;
    app->shouldRender = true;

    int windowWidth = 900, windowHeight = 600;
    int frameWidth = 640, frameHeight = 480;

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    app->window = SDL_CreateWindow("mocha", windowWidth, windowHeight, flags);
    if (app->window == nullptr)
        log(ERROR, SDL_GetError());
    SDL_ShowWindow(app->window);

    app->context = SDL_GL_CreateContext(app->window);
    if (app->context == nullptr)
        log(ERROR, SDL_GetError());

    int numCameras = 0;
    SDL_CameraID* cameraIds = SDL_GetCameras(&numCameras);
    if (numCameras == 0)
        log(ERROR, SDL_GetError());

    SDL_CameraSpec spec = {
        .format = SDL_PIXELFORMAT_RGB24,
        .colorspace = SDL_COLORSPACE_SRGB,
        .width = frameWidth,
        .height = frameHeight,
        .framerate_numerator = 1000,
        .framerate_denominator = 60
    };
    app->canReadCamera = false;
    app->camera = SDL_OpenCamera(cameraIds[0], &spec);
    if (app->camera == nullptr)
        log(ERROR, SDL_GetError());

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
        log(ERROR, "Couldn't initialize glad!");
    glDebugMessageCallback(debugCallback, 0);

    try {
        app->engine.init(windowWidth, windowHeight, frameWidth, frameHeight);
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

    *state = app;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* state)
{
    App* app = (App*)state;
    if (!app->shouldRender)
        return SDL_APP_CONTINUE;

    if (app->canReadCamera) {
        SDL_Surface* frame = SDL_AcquireCameraFrame(app->camera, nullptr);
        if (frame) {
            app->engine.handleWebcamFrame(frame->pixels);
            SDL_ReleaseCameraFrame(app->camera, frame);
        }
    }

    unsigned int startMs = SDL_GetTicks();

    app->engine.draw(double(SDL_GetTicks()) / 1000.0);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    app->engine.drawGUI();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(app->window);
    unsigned int endMs = SDL_GetTicks();

    float fps = std::floor(1000.0 / float(endMs - startMs));
    app->engine.setFPS(int(fps));

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

        case SDL_EVENT_WINDOW_MINIMIZED:
            app->shouldRender = false;
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            app->shouldRender = true;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_LEFT)
                app->engine.rotateCamera(true);
            if (event->key.key == SDLK_RIGHT)
                app->engine.rotateCamera(false);
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            app->engine.handleClick(event->button.x, event->button.y);
            break;

        case SDL_EVENT_MOUSE_WHEEL:
        {
            int x = event->wheel.mouse_x;
            int y = event->wheel.mouse_y;
            if (app->engine.insideViewport(x, y))
                app->engine.zoomCamera(event->wheel.y);
            break;
        }

        case SDL_EVENT_CAMERA_DEVICE_DENIED:
            log(WARN, "Webcam usage is required");
            return SDL_APP_FAILURE;

        case SDL_EVENT_CAMERA_DEVICE_APPROVED:
            app->canReadCamera = true;
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
    SDL_CloseCamera(app->camera);
    SDL_GL_DestroyContext(app->context);
    SDL_DestroyWindow(app->window);

    app->engine.cleanup();
    delete app;
}
