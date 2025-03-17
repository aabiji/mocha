#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

Camera::Camera(glm::vec3 _target, float _distance)
{
    target = _target;
    distance = _distance;
    position = glm::vec3(0.0, 0.0, _distance);
    angle = pitch = 0;
    updatePosition();
}

glm::vec3 Camera::getPosition() { return position; }

glm::mat4 Camera::getView()
{
    glm::vec3 up = glm::vec3(0, 1, 0);
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::getProjection(double w, double h)
{
    // 45 degree field of view
    return glm::perspective(0.785398, w / h, 0.1, 100.0);
}

void Camera::zoom(int direction)
{
    distance = std::max(3.0f, std::min(distance + direction, 25.0f));
    updatePosition();
}

void Camera::rotate(float xoffset, float yoffset)
{
    // Tilt up/down
    pitch += yoffset * 0.5;
    pitch = std::max(-5.0f, std::min(pitch, 5.0f));

    // Rock left/right
    angle += xoffset * 0.5;
    if (angle < 0)   angle += 360;
    if (angle > 360) angle -= 360;

    updatePosition();
}

void Camera::updatePosition()
{
    float a = glm::radians(angle);
    float b = glm::radians(pitch);
    position = glm::vec3(
        sin(a) * cos(b) * distance,
        sin(b) * distance,
        cos(a) * cos(b) * distance
    );
}
