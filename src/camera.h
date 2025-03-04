#pragma once

#include <glm/glm.hpp>

enum Direction { Up, Down, Left, Right, Forward, Backward };

class Camera {
public:
  Camera(glm::vec3 initial, glm::vec3 _worldUp, float _sensitivity);

  void move(Direction direction, float speed);
  void rotate(double xoffset, double yoffset);
  void zoom(double offset);

  glm::mat4 getView();

  glm::vec3 position;
  glm::vec3 front;
  double fieldOfView;
private:
  void updateVectors();

  glm::vec3 up;
  glm::vec3 right;

  float pitch;
  float yaw;

  float sensitivity;
  glm::vec3 worldUp;
};