#pragma once

#include "camera.h"
#include "framebuffer.h"
#include "model.h"
#include "movenet.h"
#include "pool.h"
#include "skybox.h"

class Engine
{
public:
    void init(int width, int height, int frameWidth, int frameHeight);
    void cleanup();

    void draw(float timeInSeconds);
    void drawGUI();

    void setFPS(float _fps) { fps = _fps; }

    bool insideViewport(int mouseX, int mouseY);
    void resizeViewport(int width, int height);

    void rotateCamera(bool left);
    void zoomCamera(int deltaY);

    void handleClick(int mouseX, int mouseY);
    void handleWebcamFrame(void* framePixels);
private:
    void loadModel(std::string name, std::string path, std::string base);
    void drawModels(bool isidOverlay, double timeInSeconds);
    void initLights();

    void drawModelInfo();
    void drawWebcamVisualization();

    int fps;
    int sidePanelWidth;
    glm::vec2 viewport;
    int selectedModel;

    Camera camera;
    Skybox skybox;
    Shader shader;

    Texture webcamFrame;
    glm::vec2 frameSize;

    MoveNet movenet;
    std::vector<Keypoint> keypoints;

    TextureLoader textureLoader;
    Framebuffer idOverlay; // Model id overlay

    std::vector<Model> models;
    ThreadPool pool;
};
