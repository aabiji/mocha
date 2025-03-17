#include <iostream>

#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

#include <SDL3/SDL.h>

#include "camera.h"
#include "model.h"

#define normalizeRGB(r, g, b) glm::vec3(r / 255.0, g / 255.0, b / 255.0)

// TODO:
// Add a floor
// Each model should have its own matrix (that we load from the file AND that we define)
// Improve the lighting -- add directional and point light (look at the lighting blender uses by default)
// Refactor and tidy up the code
// Start researching skeletal animation
// Clone mediapipe and start going through it

enum LogType { DEBUG = 32, WARN = 33, ERROR = 31 };
void log(LogType type, std::string message, bool newline = true)
{
    std::cout << "[\x1b[1;" << type << "m";
    std::cout << (type == DEBUG ? "DEBUG" : type == WARN ? "WARN" : "ERROR");
    std::cout << "\x1b[;39m] " << message;
    if (newline) std::cout << "\n";
    if (type == ERROR) exit(1);
}

void debugCallback(
    unsigned int source, unsigned int type, unsigned int id,
    unsigned int severity, int length, const char *message, const void *param)
{
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)param;

    LogType t = severity == GL_DEBUG_SEVERITY_HIGH
        ? ERROR
        : severity == GL_DEBUG_SEVERITY_MEDIUM ? WARN : DEBUG;
    log(t, message, false);
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

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

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, 0);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    Shader shader;
    try {
        shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
        shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
        shader.assemble();
        shader.use();
    } catch (std::string message) {
        log(ERROR, message);
    }

    //../assets/male-model.obj
    //../assets/rigged-guy.fbx
    //../assets/Audio-R8/Models/Audi R8.fbx
    //../assets/Star Marine Trooper/StarMarineTrooper.obj
    std::vector<Model> models;
    std::vector<std::string> paths = {
        "../assets/cube.obj", "../assets/fox.glb"
    };
    try {
        shader.use();
        for (std::string path : paths) {
            Model m;
            m.load(path.c_str());
            models.push_back(m);
        }
    } catch (std::string message) {
        log(ERROR, message);
    }

    SDL_Event event;
    bool running = true;
    glm::vec3 bg = normalizeRGB(255, 220, 254);
    Camera camera(glm::vec3(0.0, 0.0, 0.0), 4);
    glm::mat4 modelMatrix = glm::mat4(1.0);

    while (running) {
        while (SDL_PollEvent(&event)) {
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
            if (event.type == SDL_EVENT_MOUSE_WHEEL)
                camera.zoom(event.wheel.y);
            if (event.type == SDL_EVENT_MOUSE_MOTION)
                camera.rotate(event.motion.xrel, event.motion.yrel);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(bg.x, bg.y, bg.z, 1.0);

        shader.use();

        shader.setMatrix("model", modelMatrix);
        shader.setMatrix("view", camera.getView());
        shader.setMatrix("projection", camera.getProjection(width, height));

        shader.setVec3("viewPos", camera.getPosition());
        shader.setVec4("lightPos", glm::vec4(0.0, 3.0, 1.0, 0.0));

        shader.setVec3("light.direction", glm::vec3(0.5, -1.0, 0.5));
        shader.setVec3("light.color", glm::vec3(1.0, 1.0, 1.0));
        shader.setFloat("light.constant", 1.0);
        shader.setFloat("light.linear", 0.08);
        shader.setFloat("light.quadratic", 0.032);

        for (Model& m : models)
            m.draw(shader);

        SDL_GL_SwapWindow(window);
        // TODO: add stable fps
    }

    for (Model& m : models)
        m.cleanup();
    shader.cleanup();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    return 0;
}
