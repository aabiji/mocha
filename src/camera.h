#pragma once

#include <glm/glm.hpp>

enum Direction { Up, Down, Left, Right, Forward, Backward };

class Camera {
public:
  Camera(glm::vec3 initial, glm::vec3 _worldUp, float _sensitivity);

  glm::mat4 getView();
  double getFieldOfView();

  void move(Direction direction, float speed);
  void rotate(double xoffset, double yoffset);
  void zoom(double offset);

private:
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 front;
  glm::vec3 position;

  float pitch;
  float yaw;
  float fieldOfView;

  float sensitivity;
  glm::vec3 worldUp;

  void updateVectors();
};