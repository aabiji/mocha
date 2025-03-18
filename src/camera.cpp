#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

Camera::Camera(glm::vec3 _target, float _distance)
{
    target = _target;
    distance = _distance;
    position = glm::vec3(0.0, 0.0, _distance);
    pitch = yaw = 0;
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
    return glm::perspective(0.785398, w / h, 0.1, 100.0); // 45 degree field of view
}

void Camera::zoom(int direction)
{
    distance = std::max(3.0f, std::min(distance + direction, 15.0f));
    updatePosition();
}

void Camera::rotate(float offset)
{
    yaw += offset;
    if (yaw < 0)   yaw += 360;
    if (yaw > 360) yaw -= 360;
    updatePosition();
}

void Camera::updatePosition()
{
    float a = glm::radians(yaw);
    float b = glm::radians(pitch);
    position = glm::vec3(
        sin(a) * cos(b) * distance,
        sin(b) * distance + 0.5,
        cos(a) * cos(b) * distance
    );
}
