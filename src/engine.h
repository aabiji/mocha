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
    void init(glm::vec4 view);
    void cleanup();

    void draw(float timeInSeconds);
    void drawGUI();

    void setFPS(float _fps) { fps = _fps; }

    void mouseHover(int mouseX, int mouseY);

    bool insideViewport(int mouseX, int mouseY);
    void resizeViewport(int width, int height);

    void rotateCamera(bool left);
    void zoomCamera(int deltaY);
private:
    void loadModel(std::string name, std::string path, std::string base);

    void initLights();

    float fps;
    glm::vec4 viewport;

    Camera camera;
    Shader shader;
    ThreadPool pool;

    std::vector<Model> models;
    std::vector<Light> lights;
};
