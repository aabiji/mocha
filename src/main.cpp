#include <iostream>

#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL3/SDL.h>

#include "model.h"

// TODO:
// Stress test the engine by rendering multiple models (find more, better models)
// Improve the lighting -- add directional and point light
// Better the camera -- look around with the mouse, rotate around using left and right arrow keys
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
    SDL_Window* window = SDL_CreateWindow("mocha", width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr)
        log(ERROR, "Couldn't open window!");

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr)
        log(ERROR, "Couldn't initialize an OpenGL context");

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
        log(ERROR, "Couldn't initialize glad!");

    Shader shader;
    try {
        shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
        shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
        shader.assemble();
        shader.use();
    } catch (std::string message) {
        log(ERROR, message);
    }

    Model model;
    try {
        shader.use();
        model.load("../assets/fox.glb");
    } catch (std::string message) {
        log(ERROR, message);
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, 0);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    SDL_Event event;
    bool running = true;

    glm::mat4 projection = glm::perspective(glm::radians(45.0), (double)width / (double)height, 0.1, 100.0);
    glm::mat4 modelMatrix = glm::mat4(1.0); // At the origin
    float angle = 88, radius = 5;

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
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_LEFT) {
                    angle++;
                    if (angle >= 360)
                        angle = 0;
                } else if (event.key.key == SDLK_RIGHT) {
                    angle--;
                    if (angle <= 0)
                        angle = 360;
                } else if (event.key.key == SDLK_UP) {
                    radius = std::max(1.0f, radius - 1);
                } else if (event.key.key == SDLK_DOWN) {
                    radius = std::min(10.0f, radius + 1);
                }
            }
            if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_SPACE) {
                    int polygonMode[2];
                    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
                    int mode = polygonMode[0] == GL_LINE ? GL_FILL : GL_LINE;
                    glPolygonMode(GL_FRONT_AND_BACK, mode);
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.64, 0.64, 0.64, 1.0);

        shader.use();

        // Camera pointing towards the origin that can rotate around it
        glm::vec3 cameraPosition = glm::vec3(radius * cos(glm::radians(angle)), 0.0, radius * sin(glm::radians(angle)));
        glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        shader.setMatrix("projection", glm::value_ptr(projection));
        shader.setMatrix("view", glm::value_ptr(view));
        shader.setMatrix("model", glm::value_ptr(modelMatrix));

        shader.setVector("viewPos", glm::value_ptr(cameraPosition));
        shader.setVector("lightPos", glm::value_ptr(glm::vec4(0.0, 3.0, 1.0, 0.0)), 4);

        shader.setVector("light.direction", glm::value_ptr(glm::vec3(0.5, -1.0, 0.5)));
        shader.setVector("light.color", glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
        shader.setFloat("light.constant", 1.0);
        shader.setFloat("light.linear", 0.08);
        shader.setFloat("light.quadratic", 0.032);

        model.draw(shader);

        SDL_GL_SwapWindow(window);
        // TODO: add fps
    }

    model.cleanup();
    shader.cleanup();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    return 0;
}
