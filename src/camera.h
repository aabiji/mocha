#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    void init(glm::vec3 _target, float _distance)
    {
        target = _target;
        distance = _distance;
        position = glm::vec3(0.0, 0.0, _distance);
        pitch = yaw = 0;
        updatePosition();
    }

    glm::vec3 getPosition() { return position; }

    // Get the view matrix
    glm::mat4 getView()
    {
        glm::vec3 up = glm::vec3(0, 1, 0);
        return glm::lookAt(position, target, up);
    }

    // Get the projection matrix
    glm::mat4 getProjection(double w, double h)
    {
        return glm::perspective(0.785398, w / h, 0.1, 100.0); // 45 degree field of view
    }

    // Zoom in (direction = 1) and out (direction = -1)
    void zoom(int direction)
    {
        distance = std::max(3.0f, std::min(distance + direction, 15.0f));
        updatePosition();
    }

    // Rotate the camera horizontally around the target position
    void rotate(float offset)
    {
        yaw += offset;
        if (yaw < 0)   yaw += 360;
        if (yaw > 360) yaw -= 360;
        updatePosition();
    }
private:
    void updatePosition()
    {
        float a = glm::radians(yaw);
        float b = glm::radians(pitch);
        position = glm::vec3(
            sin(a) * cos(b) * distance,
            sin(b) * distance + 0.5,
            cos(a) * cos(b) * distance
        );
    }

    float distance; // Distance from the target
    float pitch; // Vertical tilt (degrees)
    float yaw; // Angle the camera is around the target (degrees)

    glm::vec3 position;
    glm::vec3 target; // Point the camera's looking at
};
