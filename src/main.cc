#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

void resizeCallback(GLFWwindow *window, int width, int height) {
  (void)window; // unused
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE))
    glfwSetWindowShouldClose(window, true);
}

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int width = 800, height = 600;
  GLFWwindow *window = glfwCreateWindow(width, height, "App", nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Couldn't create a window\n";
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Couldn't initialize GLAD\n";
    return -1;
  }

  glViewport(0, 0, width, height);
  glfwSetFramebufferSizeCallback(window, resizeCallback);

  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
