#pragma once

#include "camera.h"
#include "framebuffer.h"
#include "model.h"
#include "pool.h"
#include "skybox.h"

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

    void handleClick(int mouseX, int mouseY);
private:
    void loadModel(std::string name, std::string path, std::string base);
    void drawModels(bool isFramebuffer, double timeInSeconds);
    void initLights();

    int fps;
    int sidePanelWidth;
    glm::vec2 viewport;
    int selectedModel;

    Camera camera;
    ThreadPool pool;
    Shader shader;
    Framebuffer framebuffer;
    std::vector<Model> models;
    Skybox skybox;
};
