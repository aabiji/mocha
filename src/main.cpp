#include <iostream>

#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL3/SDL.h>

#include "model.h"
#include "shader.h"

// TODO: implement a better logger
void debugCallback(
    unsigned int source, unsigned int type, unsigned int id,
    unsigned int severity, int length, const char *message, const void *param)
{
    (void)source;
    (void)type;
    (void)id;
    (void)severity;
    (void)length;
    (void)param;
    std::cout << "DEBUG: " << message << "\n";
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int width = 800, height = 600;
    SDL_Window* window = SDL_CreateWindow("mocha", width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cout << "Couldn't open window!\n";
        return -1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        std::cout << "Couldn't initialize an OpenGL context\n";
        return -1;
    }

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        std::cout << "Couldn't initialize glad!\n";
        return -1;
    }

    Shader shader;
    try {
        shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
        shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
        shader.assemble();
        shader.use();
    } catch (std::string message) {
        std::cout << message << "\n";
        return -1;
    }

    Model model;
    try {
        model.load("../assets/cube.obj");
    } catch (std::string message) {
        std::cout << message << "\n";
        return -1;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, 0);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    SDL_Event event;
    bool running = true;

    glm::mat4 projection = glm::perspective(glm::radians(45.0), (double)width / (double)height, 0.1, 100.0);
    glm::mat4 modelMatrix = glm::mat4(1.0); // At the origin
    glm::mat4 normal = glm::inverse(glm::transpose(glm::mat3(modelMatrix)));
    float angle = 0, radius = 15;

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
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearColor(0.64, 0.64, 0.64, 1.0);

        shader.use();

        // Camera pointing towards the origin that can rotate around it
        glm::vec3 cameraPosition = glm::vec3( radius * cos(glm::radians(angle)), 0.0, radius * sin(glm::radians(angle)));
        glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 transform = projection * view * modelMatrix;
        shader.setMatrix("transform", glm::value_ptr(transform));
        shader.setMatrix("normalMatrix", glm::value_ptr(normal));

        model.draw();

        SDL_GL_SwapWindow(window);
    }

    // TODO: cleanup opengl
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    return 0;
}