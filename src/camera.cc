#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

Camera::Camera(glm::vec3 initial, glm::vec3 _worldUp, float _sensitivity) {
  sensitivity = _sensitivity;
  position = initial;
  worldUp = _worldUp;
  fieldOfView = 45;
  pitch = 0.0;
  yaw = -90.0;
  updateVectors();
}

glm::mat4 Camera::getView() {
  return glm::lookAt(position, position + front, up);
}

double Camera::getFieldOfView() {
  return fieldOfView;
}

void Camera::move(Direction direction, float speed) {
  if (direction == Direction::Up)       position += up * speed;
  if (direction == Direction::Down)     position -= up * speed;
  if (direction == Direction::Left)     position -= right * speed;
  if (direction == Direction::Right)    position += right * speed;
  if (direction == Direction::Forward)  position -= front * speed;
  if (direction == Direction::Backward) position += front * speed;
}

void Camera::rotate(double xOffset, double yOffset) {
  pitch = std::max(-89.9, std::min(pitch + yOffset * sensitivity, 89.9));
  yaw += xOffset * sensitivity;
  updateVectors();
}

void Camera::zoom(double offset) {
  fieldOfView = std::max(1.0, std::min(fieldOfView - offset, 45.0));
}

void Camera::updateVectors() {
  glm::vec3 direction = glm::vec3(0.0, 0.0, 0.0);
  direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
  direction.y = std::sin(glm::radians(pitch));
  direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

  front = glm::normalize(direction);
  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}