#pragma once

#include "camera.h"
#include "model.h"
#include "pool.h"

struct Light
{
    // Vectors in the std140 format need to be multiples of 4
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 position;
    // Constant, linear, quadratic (attenuation value) (see shaders/shared.glsl)
    float c, l, q;
};

class Engine
{
public:
    void init(int width, int height, int panelSize);
    void cleanup();

    void draw(float timeInSeconds);
    void drawGUI();

    void setFPS(float _fps) { fps = _fps; }

    bool insideViewport(int mouseX, int mouseY);
    void resizeViewport(int width, int height);

    void rotateCamera(bool left);
    void zoomCamera(int deltaY);
private:
    void loadModel(std::string name, std::string path, std::string base);

    void initLights();

    int fps;
    int sidePanelWidth;
    glm::vec2 windowSize;

    Camera camera;
    Shader shader;
    ThreadPool pool;

    std::vector<Model> models;
    std::vector<Light> lights;
};
