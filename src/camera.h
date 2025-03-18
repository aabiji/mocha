#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera(glm::vec3 _target, float _distance);

    glm::vec3 getPosition();
    glm::mat4 getView(); // Get the view matrix
    glm::mat4 getProjection(double w, double h); // Get the projection matrix

    void zoom(int direction); // Zoom in (1) and out (-1)

    // Rotate the camera horizontally around the target position
    void rotate(float offset);
private:
    void updatePosition();

    float distance; // Distance from the target
    float pitch; // Vertical tilt (degrees)
    float yaw; // Angle the camera is around the target (degrees)

    glm::vec3 position;
    glm::vec3 target; // Point the camera's looking at
};
