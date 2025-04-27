#include "movenet.h"
#define GLAD_GL_IMPLEMENTATION 1
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include "engine.h"
#include "log.h"

struct Light
{
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec3 position;
    float c, l, q;
};

void Engine::init(int width, int height, int frameWidth, int frameHeight)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    sidePanelWidth = width / 4;
    resizeViewport(width, height);

    skybox.init("../assets/dancing_hall_4k.hdr", "../assets/dancing_hall/");
    camera.init(glm::vec3(0.0), 15, viewport.x, viewport.y);
    idOverlay.init(viewport.x, viewport.y);

    movenet.init("../assets/movenet_singlepose.tflite");
    keypoints.clear();
    frameSize = glm::vec2(frameWidth, frameHeight);
    webcamFrame = Texture(frameSize.x, frameSize.y);
    webcamFrame.init();

    shader.load(GL_VERTEX_SHADER, "../src/shaders/default/vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../src/shaders/default/fragment.glsl");
    shader.assemble();
    shader.use();

    initLights();
    shader.createBuffer("mvp", 2, sizeof(MVPTransforms));

    pool.init(3);
    loadModel("player", "../assets/characters/Knight.fbx", "../assets/characters/");
    selectedModel = -1;
}

void Engine::cleanup()
{
    for (Model& model : models)
        model.cleanup();
    textureLoader.cleanup();
    webcamFrame.cleanup();
    idOverlay.cleanup();
    shader.cleanup();
    skybox.cleanup();
    pool.terminate();
}

void Engine::rotateCamera(bool left) { camera.rotate(left ? -1 : 1); }
void Engine::zoomCamera(int deltaY) { camera.zoom(deltaY); }

bool Engine::insideViewport(int mouseX, int mouseY)
{
    return mouseX >= sidePanelWidth &&
           mouseX <= viewport.x + sidePanelWidth &&
           mouseY >= 0 && mouseY <= viewport.y;
}

void Engine::resizeViewport(int width, int height)
{
    sidePanelWidth = width / 4;
    viewport.y = height;
    viewport.x = width - sidePanelWidth;
}

// Select the model that the mouse is hovering upon
void Engine::handleClick(int mouseX, int mouseY)
{
    int x = mouseX - sidePanelWidth;
    int y = viewport.y - mouseY;
    if (x < 0 || y < 0) return; // Invalid coordinates

    float pixel[4] = {100, 100, 100, 100};
    idOverlay.readPixel(x, y, pixel);
    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
        selectedModel = -1;
        return; // The pixel is the same as the clear color
    }

    // Map back from a range of 0 to 1 to a range of 0 to INT_MAX
    int modelIndex = int(ceil((1.0 / pixel[0]))) - 1;
    selectedModel = modelIndex;
}

void Engine::handleWebcamFrame(void* framePixels)
{
    if (!movenet.ready()) return;

    // Make of a copy of the frame so that we can free
    // the surface sdl gives us right away
    int allocationSize = frameSize.x * frameSize.y * 3 * sizeof(unsigned char);
    unsigned char* copy = (unsigned char*)malloc(allocationSize);
    memcpy(copy, framePixels, allocationSize);

    webcamFrame.write(0, 0, copy);

    pool.dispatch([copy, this](){
        keypoints = movenet.runInference(copy, frameSize);
        for (Keypoint kp : keypoints) {
            if (kp.detected())
                std::cout << "KEYPOINT: " << kp.x << " " << kp.y << "\n";
        }
        free(copy);
    });
}

void Engine::loadModel(std::string name, std::string path, std::string base)
{
    pool.dispatch([&, name, path, base] {
        try {
            Model model(&textureLoader, name, path, base);
            models.push_back(model);
        } catch (std::string msg) {
            log(ERROR, msg);
        }
    });
}

void Engine::initLights()
{
    glm::vec3 positions[] = {
        glm::vec3( 0.0, 1.0, -3.0),
        glm::vec3( 0.0, 1.0,  3.0),
        glm::vec3(-3.0, 1.0,  0.0),
    };

    Light lights[3];
    for (int i = 0; i < 3; i++) {
        lights[i] = {
            .color = glm::vec3(0.8),
            .position = positions[i],
            .c = 1.0,
            .l = 0.08,
            .q = 0.032
        };
    }

    shader.createBuffer("lights", 0, sizeof(lights));
    shader.writeBuffer("lights", &lights, 0, sizeof(lights));
}

void Engine::drawGUI()
{
    ImGui::NewFrame();
    drawModelInfo();
    drawWebcamVisualization();
    ImGui::Render();
}

void Engine::drawModelInfo()
{
    ImGui::Begin("Model info", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::Text("%s", (std::to_string(fps) + " FPS").c_str());
    ImGui::SetWindowSize(ImVec2(sidePanelWidth, (viewport.y / 3) * 2));
    ImGui::SetWindowPos(ImVec2(0, 0));

    if (selectedModel != -1) {
        assert(selectedModel < int(models.size()));
        Model& model = models[selectedModel];
        auto animations = model.animationNames();
        size_t current = model.getCurrentAnimation();

        if (animations.size() > 0) {
            ImGui::Separator();
            ImGui::Text("%s", model.getName().c_str());
            ImGui::SameLine();
            if (ImGui::Button(model.animationPlaying() ? "Pause" : "Play"))
                model.toggleAnimation();

            if (ImGui::BeginChild("##list", ImVec2(sidePanelWidth - 10, 0))) {
                for (size_t i = 0; i < animations.size(); i++) {
                    bool selected = current == i;
                    if (ImGui::Selectable(animations[i].c_str(), selected))
                        model.setCurrentAnimation(i);
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndChild();
            }
        }
    }

    ImGui::End();
}

void Engine::drawWebcamVisualization()
{
    double third = viewport.y / 3;
    ImVec2 size = ImVec2(sidePanelWidth, third);
    ImVec2 base = ImVec2(-3, third * 2 - 3);
    ImU32 color = ImGui::GetColorU32(ImVec4(0, 255, 0, 255));
    auto drawList = ImGui::GetForegroundDrawList();

    ImGui::Begin("Webcam", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetWindowSize(size);
    ImGui::SetWindowPos(base);

    ImGui::Image((ImTextureID)*webcamFrame.id, size);

    // Draw the skeleton constructed from the keypoints on top of the image
    for (Keypoint kp : keypoints) {
        if (!kp.detected()) continue;
        glm::vec2 p = movenet.getPosition(kp.x, kp.y, size.x, size.y);
        drawList->AddCircleFilled({base.x + p.x, base.y + p.y}, 3, color);
    }

    for (auto [i, j] : keypointConnections) {
        if (i >= int(keypoints.size()) || j >= int(keypoints.size()))
            continue; // Out of range
        if (!keypoints[i].detected() || !keypoints[j].detected())
            continue; // Incomplete connection

        glm::vec2 p1 = movenet.getPosition(
            keypoints[i].x, keypoints[i].y, size.x, size.y);
        glm::vec2 p2 = movenet.getPosition(
            keypoints[j].x, keypoints[j].y, size.x, size.y);
        drawList->AddLine(
            {base.x + p1.x, base.y + p1.y},
            {base.x + p2.x, base.y + p2.y},
            color
        );
    }

    ImGui::End();
}

void Engine::drawModels(bool isFramebuffer, double timeInSeconds)
{
    if (isFramebuffer)
        idOverlay.bind();
    else
        idOverlay.unbind();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glViewport(isFramebuffer ? 0 :  sidePanelWidth, 0, viewport.x, viewport.y);

    shader.use();
    shader.set<int>("isFramebuffer", isFramebuffer);
    MVPTransforms transforms = camera.getMVPTransforms();
    shader.writeBuffer("mvp", &transforms, 0, sizeof(MVPTransforms));

    for (unsigned int i = 0; i < models.size(); i++) {
        Model& model = models[i];
        if (model.isCalled("player")) {
            model.setSize(glm::vec3(0.0, 5.0, 0.0), true);
            model.setPosition(glm::vec3(0.0, -5.0, 0.0));
        }
        shader.set<float>("modelId", i + 1);
        model.draw(shader, timeInSeconds);
    }
}

void Engine::draw(float timeInSeconds)
{
    drawModels(true, timeInSeconds);
    drawModels(false, timeInSeconds);
    skybox.draw(camera.getProjection(), camera.getViewWithoutTranslation());
}
