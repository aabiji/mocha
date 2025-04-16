#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct MVPTransforms
{
    glm::mat4 view;
    glm::mat4 projection;
    alignas(16) glm::vec3 viewPosition;
};

class Camera
{
public:
    void init(glm::vec3 _target, float _distance, double width, double height)
    {
        target = _target;
        distance = _distance;
        position = glm::vec3(0.0, 0.0, _distance);
        pitch = yaw = 0;
        w = width;
        h = height;
        updatePosition();
    }

    MVPTransforms getMVPTransforms()
    {
        MVPTransforms values;

        glm::vec3 up = glm::vec3(0, 1, 0);
        values.view = glm::lookAt(position, target, up);

        double fov = 0.785398; // 45 degres in radians
        values.projection = glm::perspective(fov, w / h, 0.1, 100.0);

        values.viewPosition = position;
        return values;
    }

    glm::mat4 getProjection()
    {
        double fov = 0.785398; // 45 degres in radians
        return glm::perspective(fov, w / h, 0.1, 100.0);
    }

    glm::mat4 getViewWithoutTranslation()
    {
        glm::vec3 up = glm::vec3(0, 1, 0);
        glm::mat4 view = glm::lookAt(position, target, up);
        return glm::mat4(glm::mat3(view));
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

    double w, h; // Window size

    float distance; // Distance from the target
    float pitch; // Vertical tilt (degrees)
    float yaw; // Angle the camera is around the target (degrees)

    glm::vec3 position;
    glm::vec3 target; // Point the camera's looking at
};
