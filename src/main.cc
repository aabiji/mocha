#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "shaders.h"

void resizeCallback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

void keyCallback(GLFWwindow *window, int key, int scanCode, int action,
                 int mods) {
  (void)mods;
  (void)scanCode;

  // exit
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // toggle wireframe
  if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
    int polygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);
    int newMode = polygonMode[0] == GL_LINE ? GL_FILL : GL_LINE;
    glPolygonMode(GL_FRONT_AND_BACK, newMode);
  }
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
  glfwSetKeyCallback(window, keyCallback);

  Shaders shaders;
  try {
    shaders.load(GL_VERTEX_SHADER, "../src/vertex.glsl");
    shaders.load(GL_FRAGMENT_SHADER, "../src/fragment.glsl");
    shaders.assemble();
  } catch (std::string str) {
    std::cout << str;
    return -1;
  }

  float vertices[] = {
      -0.5, -0.5, 0.0, 1.0, 0.0, 0.0, // bottom left, red
      0.5,  -0.5, 0.0, 0.0, 1.0, 0.0, // bottom right, green
      0.0,  0.5,  0.0, 0.0, 0.0, 1.0, // top, blue
  };
  unsigned int indexes[] = {0, 1, 2};
  unsigned int vao, vbo, ebo;

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes,
               GL_STATIC_DRAW);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shaders.use();
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, sizeof(vertices), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
