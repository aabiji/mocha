#include <iostream>

// TODO: make the background gray
// TODO: make passing structs to the shader more ergonomic (uniform buffer object)
// TODO: remove all camera movement (just rotation around the origin)
#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

#include <SDL3/SDL.h>

void debugCallback(
  unsigned int source, unsigned int type, unsigned int id,
  unsigned int severity, int length, const char *message, const void *param
) {
  (void)source;
  (void)type;
  (void)id;
  (void)severity;
  (void)length;
  (void)param;
  std::cout << message << "\n";
}

int main() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window* window = SDL_CreateWindow("mocha", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(debugCallback, 0);

  glViewport(0, 0, 800, 600);
  glEnable(GL_DEPTH_TEST);

  SDL_Event event;
  bool running = true;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
        break;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED)
        glViewport(0, 0, event.window.data1, event.window.data2);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0, 0.5, 1.0, 1.0);
    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);
  return 0;
}