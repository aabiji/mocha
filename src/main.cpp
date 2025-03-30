#include <iostream>
#include <format>

#define IMGUI_USER_CONFIG "imgui_config.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <SDL3/SDL.h>

#include "camera.h"
#include "model.h"
#include "pool.h"

#define normalizeRGB(r, g, b) glm::vec3(r / 255.0, g / 255.0, b / 255.0)

struct Light
{
    // Vectors in the std140 format need to be multiples of 4
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 position;
    // Constant, linear, quadratic (attenuation value) (see shaders/shared.glsl)
    float c, l, q;
};

enum LogType { DEBUG = 32, WARN = 33, ERROR = 31 };
void log(LogType type, std::string message)
{
    std::cout << "[\x1b[1;" << type << "m";
    std::cout << (type == DEBUG ? "DEBUG" : type == WARN ? "WARN" : "ERROR");
    std::cout << "\x1b[;39m] " << message << "\n";
    if (type == ERROR) exit(1);
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

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    int width = 800, height = 600;
    long unsigned int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    SDL_Window* window = SDL_CreateWindow("mocha", width, height, flags);
    if (window == nullptr)
        log(ERROR, SDL_GetError());

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr)
        log(ERROR, SDL_GetError());

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
        log(ERROR, "Couldn't initialize glad!");

    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext(nullptr);
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->AddFontFromFileTTF("../assets/Roboto-Regular.ttf", 15);

    ImGui_ImplSDL3_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 460");

    Shader shader;
    try {
        shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
        shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
        shader.assemble();
        shader.use();
    } catch (std::string message) {
        log(ERROR, message);
    }

    std::vector<Model> models;
    std::vector<std::string> paths = {
        //"../assets/cube.glb",
        "../assets/Skeleton_Mage.glb"
    };
    ThreadPool pool(3);
    for (std::string path : paths) {
        pool.dispatch([&, path] {
            try {
                Model m(path, "../assets/");
                if (path == "../assets/cube.glb") {
                    // Scale and position the floor
                    m.setSize(glm::vec3(10.0, 0.5, 10.0), false);
                    m.setPosition(glm::vec3(0.0, -0.5, 0.0));
                } else {
                    // Scale the model to have a height of
                    // 1.0 and position it on top of the floor
                    m.setSize(glm::vec3(0.0, 1.0, 0.0), true);
                    m.setPosition(glm::vec3(0.0, 0.0, 0.0));
                }
                models.push_back(m);
            } catch (std::string message) {
                log(ERROR, message);
            }
        });
    }

    SDL_Event event;
    bool running = true;
    std::string fpsCounter = "";
    glm::vec3 bg = normalizeRGB(70, 70, 70);
    Camera camera(glm::vec3(0.0, 0.0, 0.0), 3);

    glm::vec3 positions[] = {
        glm::vec3(0.0, 1.0, -3.0), glm::vec3(0.0, 1.0, 3.0),
        glm::vec3(-3.0, 1.0, 0.0), glm::vec3(5.0, 1.0, 0.0)
    };
    glm::vec3 colors[] = {
        normalizeRGB(180, 180, 180), normalizeRGB(200, 200, 200),
        normalizeRGB(212, 212, 212), normalizeRGB(212, 212, 212)
    };
    Light lights[4];
    for (int i = 0; i < 4; i++) {
        lights[i] = {
            .color = colors[i], .position = positions[i],
            .c = 1.0, .l = 0.08, .q = 0.032
        };
    }
    unsigned int lightsBuffer = shader.createBuffer(1, sizeof(lights));
    shader.updateBuffer(lightsBuffer, sizeof(lights), &lights);

    while (running) {
        unsigned int startMs = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                width = event.window.data1;
                height = event.window.data2;
                glViewport(0, 0, width, height);
            }
            if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_SPACE) {
                int polygonMode[2];
                glGetIntegerv(GL_POLYGON_MODE, polygonMode);
                int mode = polygonMode[0] == GL_LINE ? GL_FILL : GL_LINE;
                glPolygonMode(GL_FRONT_AND_BACK, mode);
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_LEFT)
                    camera.rotate(-1);
                if (event.key.key == SDLK_RIGHT)
                    camera.rotate(1);
            }
            if (event.type == SDL_EVENT_MOUSE_WHEEL)
                camera.zoom(event.wheel.y);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(bg.x, bg.y, bg.z, 1.0);

        shader.use();

        shader.set<glm::mat4>("view", camera.getView());
        shader.set<glm::mat4>("projection", camera.getProjection(width, height));
        shader.set<glm::vec3>("viewPosition", camera.getPosition());

        for (Model& m : models)
            m.draw(shader, double(SDL_GetTicks()) / 1000.0);

        // Draw the UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Info");
        ImGui::Text("%s", fpsCounter.c_str());
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        unsigned int endMs = SDL_GetTicks();
        float fps = std::floor(1000.0 / float(endMs - startMs));
        fpsCounter = std::format("FPS: {}", fps);
    }

    for (Model& m : models)
        m.cleanup();
    shader.cleanup();
    pool.terminate();

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    return 0;
}
