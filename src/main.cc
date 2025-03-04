#include <iostream>
#include <format>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "camera.h"
#include "shader.h"

// TODO: use Uniform Buffer Objects to pass constant data
// into shaders https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL

double windowWidth = 800, windowHeight = 600;
bool firstCapture = false;
double lastX = 400, lastY = 300;
Camera camera(glm::vec3(0.0, 0.0, 5.0), glm::vec3(0.0, 1.0, 0.0), 0.1);

float vertices[] = {
  -0.5, -0.5, -0.5,  0.0,  0.0, -1.0,  0.0,  0.0,
   0.5, -0.5, -0.5,  0.0,  0.0, -1.0,  1.0,  0.0,
   0.5,  0.5, -0.5,  0.0,  0.0, -1.0,  1.0,  1.0,
   0.5,  0.5, -0.5,  0.0,  0.0, -1.0,  1.0,  1.0,
  -0.5,  0.5, -0.5,  0.0,  0.0, -1.0,  0.0,  1.0,
  -0.5, -0.5, -0.5,  0.0,  0.0, -1.0,  0.0,  0.0,
  -0.5, -0.5,  0.5,  0.0,  0.0,  1.0,  0.0,  0.0,
   0.5, -0.5,  0.5,  0.0,  0.0,  1.0,  1.0,  0.0,
   0.5,  0.5,  0.5,  0.0,  0.0,  1.0,  1.0,  1.0,
   0.5,  0.5,  0.5,  0.0,  0.0,  1.0,  1.0,  1.0,
  -0.5,  0.5,  0.5,  0.0,  0.0,  1.0,  0.0,  1.0,
  -0.5, -0.5,  0.5,  0.0,  0.0,  1.0,  0.0,  0.0,
  -0.5,  0.5,  0.5, -1.0,  0.0,  0.0,  1.0,  0.0,
  -0.5,  0.5, -0.5, -1.0,  0.0,  0.0,  1.0,  1.0,
  -0.5, -0.5, -0.5, -1.0,  0.0,  0.0,  0.0,  1.0,
  -0.5, -0.5, -0.5, -1.0,  0.0,  0.0,  0.0,  1.0,
  -0.5, -0.5,  0.5, -1.0,  0.0,  0.0,  0.0,  0.0,
  -0.5,  0.5,  0.5, -1.0,  0.0,  0.0,  1.0,  0.0,
   0.5,  0.5,  0.5,  1.0,  0.0,  0.0,  1.0,  0.0,
   0.5,  0.5, -0.5,  1.0,  0.0,  0.0,  1.0,  1.0,
   0.5, -0.5, -0.5,  1.0,  0.0,  0.0,  0.0,  1.0,
   0.5, -0.5, -0.5,  1.0,  0.0,  0.0,  0.0,  1.0,
   0.5, -0.5,  0.5,  1.0,  0.0,  0.0,  0.0,  0.0,
   0.5,  0.5,  0.5,  1.0,  0.0,  0.0,  1.0,  0.0,
  -0.5, -0.5, -0.5,  0.0, -1.0,  0.0,  0.0,  1.0,
   0.5, -0.5, -0.5,  0.0, -1.0,  0.0,  1.0,  1.0,
   0.5, -0.5,  0.5,  0.0, -1.0,  0.0,  1.0,  0.0,
   0.5, -0.5,  0.5,  0.0, -1.0,  0.0,  1.0,  0.0,
  -0.5, -0.5,  0.5,  0.0, -1.0,  0.0,  0.0,  0.0,
  -0.5, -0.5, -0.5,  0.0, -1.0,  0.0,  0.0,  1.0,
  -0.5,  0.5, -0.5,  0.0,  1.0,  0.0,  0.0,  1.0,
   0.5,  0.5, -0.5,  0.0,  1.0,  0.0,  1.0,  1.0,
   0.5,  0.5,  0.5,  0.0,  1.0,  0.0,  1.0,  0.0,
   0.5,  0.5,  0.5,  0.0,  1.0,  0.0,  1.0,  0.0,
  -0.5,  0.5,  0.5,  0.0,  1.0,  0.0,  0.0,  0.0,
  -0.5,  0.5, -0.5,  0.0,  1.0,  0.0,  0.0,  1.0
};

void resizeCallback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
  windowWidth = width;
  windowHeight = height;
}

void keyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods) {
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

void mouseCallback(GLFWwindow *window, double x, double y) {
  (void)window;

  if (firstCapture) {
    firstCapture = true;
    lastX = x;
    lastY = y;
  }

  camera.rotate(x - lastX, lastY - y);
  lastX = x;
  lastY = y;
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  (void)window;
  (void)xoffset;
  camera.zoom(yoffset);
}

int loadTexture(const char *path) {
  int w, h, n;
  unsigned char *pixels = stbi_load(path, &w, &h, &n, 0);
  if (pixels == nullptr)
    throw std::format("Failed to read: {}\n", path);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int format = n == 1 ? GL_RED : n == 3 ? GL_RGB : GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, pixels);
  glGenerateMipmap(GL_TEXTURE_2D);

  free(pixels);
  return texture;
}

void debugCallback(unsigned int source, unsigned int type, unsigned int id,
                   unsigned int severity, int length, const char *message,
                   const void *param) {
  (void)source;
  (void)type;
  (void)id;
  (void)severity;
  (void)length;
  (void)param;
  std::cout << message << "\n";
}

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(int(windowWidth), int(windowHeight), "App", nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Couldn't create a window\n";
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Couldn't initialize GLAD\n";
    return -1;
  }

  glViewport(0, 0, int(windowWidth), int(windowHeight));
  glfwSetFramebufferSizeCallback(window, resizeCallback);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetScrollCallback(window, scrollCallback);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(debugCallback, 0);
  glEnable(GL_DEPTH_TEST);

  Shader shader;
  try {
    shader.load(GL_VERTEX_SHADER, "../src/shaders/vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../src/shaders/fragment.glsl");
    shader.assemble();
  } catch (std::string str) {
    std::cout << str;
    return -1;
  }

  unsigned int diffuseMap, specularMap;
  try {
    diffuseMap = loadTexture("diffuse.png");
    specularMap = loadTexture("specular.png");

    shader.use();
    shader.setInt("material.diffuse", 0);
    shader.setInt("material.specular", 1);

    shader.setFloat("material.shininess", 0.4);
    shader.setVector("light.ambient",  glm::value_ptr(glm::vec3(0.2, 0.2, 0.2)));
    shader.setVector("light.diffuse",  glm::value_ptr(glm::vec3(0.5, 0.5, 0.5)));
    shader.setVector("light.specular", glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
  } catch (std::string str) {
    std::cout << str;
    return -1;
  }

  unsigned int cubeVao, cubeVbo;
  glGenVertexArrays(1, &cubeVao);
  glBindVertexArray(cubeVao);
  glGenBuffers(1, &cubeVbo);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); // position
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1); // normal
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2); // texture coordinate

  Shader lightShader;
  try {
    lightShader.load(GL_VERTEX_SHADER, "../src/shaders/lightVertex.glsl");
    lightShader.load(GL_FRAGMENT_SHADER, "../src/shaders/lightFragment.glsl");
    lightShader.assemble();
  } catch (std::string str) {
    std::cout << str;
    return -1;
  }

  unsigned int lightVao, lightVbo;
  glGenVertexArrays(1, &lightVao);
  glBindVertexArray(lightVao);
  glGenBuffers(1, &lightVbo);
  glBindBuffer(GL_ARRAY_BUFFER, lightVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  float previousTime = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    float speed = 2.5 * (glfwGetTime() - previousTime);
    previousTime = glfwGetTime();
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
      camera.move(Direction::Forward, speed);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
      camera.move(Direction::Backward, speed);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.move(Direction::Up, speed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.move(Direction::Down, speed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.move(Direction::Right, speed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.move(Direction::Left, speed);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.use();

    // Draw the scene
    float radius = 10, angle = glfwGetTime();
    glm::mat4 lightPosition = glm::mat4(1.0f);
    lightPosition = glm::scale(lightPosition, glm::vec3(0.5, 0.5, 0.5));
    lightPosition = glm::translate(lightPosition, glm::vec3(cos(angle) * radius, 0.0, sin(angle) * radius));
    shader.setVector("light.position", glm::value_ptr(lightPosition));

    glm::mat4 projection = glm::mat4(1.0);
    projection = glm::perspective(glm::radians(camera.getFieldOfView()), windowWidth / windowHeight, 0.1, 100.0);
    shader.setMatrix("projection", glm::value_ptr(projection));

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
    model = glm::rotate(model, angle, glm::vec3(0.5, 1.0, 0.0));
    shader.setMatrix("model", glm::value_ptr(model));

    glm::mat4 view = camera.getView();
    shader.setMatrix("view", glm::value_ptr(view));
    shader.setVector("viewPosition", glm::value_ptr(camera.getPosition()));

    glm::mat4 normalMatrix = glm::transpose(glm::inverse(model));
    shader.setMatrix("normalMatrix", glm::value_ptr(normalMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);  
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    glBindVertexArray(cubeVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Draw the light source
    lightShader.use();
    glBindVertexArray(lightVao);
    glm::mat4 matrix = projection * camera.getView() * lightPosition;
    lightShader.setMatrix("transform", glm::value_ptr(matrix));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}