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

void Engine::init(int width, int height, int panelSize)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    sidePanelWidth = panelSize;
    resizeViewport(width, height);

    pool.init(3);
    camera.init(glm::vec3(0.0), 3);

    framebuffer.init(viewport.x, viewport.y);

    shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
    shader.assemble();
    shader.use();

    shader.createBuffer("mvp", 2, sizeof(MVPTransforms));
    initLights();

    selectedModel = -1;
    loadModel("floor", "../assets/cube.fbx", "");
    loadModel("player", "../assets/characters/Barbarian.fbx", "../assets/characters/");
}

void Engine::cleanup()
{
    for (Model& model : models)
        model.cleanup();
    shader.cleanup();
    framebuffer.cleanup();
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
    framebuffer.readPixel(x, y, pixel);
    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
        selectedModel = -1;
        return; // The pixel is the same as the clear color
    }

    // Map back from a range of 0 to 1 to a range of 0 to INT_MAX
    int modelIndex = int(ceil((1.0 / pixel[0]))) - 1;
    selectedModel = modelIndex;
}

void Engine::loadModel(std::string name, std::string path, std::string base)
{
    pool.dispatch([&, name, path, base] {
        try {
            Model model(shader, name, path, base);
            models.push_back(model);
        } catch (std::string msg) {
            log(ERROR, msg);
        }
    });
}

void Engine::initLights()
{
    glm::vec3 positions[] = {
        glm::vec3(0.0, 1.0, -3.0),
        glm::vec3(0.0, 1.0, 3.0),
        glm::vec3(-3.0, 1.0, 0.0),
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

    int flags = ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove;
    ImGui::Begin("Info", nullptr, flags);

    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImVec2(sidePanelWidth, viewport.y));

    ImGui::Text("%s", ("FPS: " + std::to_string(fps)).c_str());
    if (selectedModel == -1) {
        ImGui::End();
        ImGui::Render();
        return;
    }

    Model& model = models[selectedModel];
    auto animations = model.animationNames();
    size_t current = model.getCurrentAnimation();
    if (animations.size() == 0) { // No animations to play
        ImGui::End();
        ImGui::Render();
        return;
    }

    ImGui::Separator();
    ImGui::Text(model.getName().c_str());
    ImGui::SameLine();
    if (ImGui::Button(model.animationPlaying() ? "Pause" : "Play"))
        model.toggleAnimation();

    if (ImGui::BeginChild("##list", ImVec2(240, 0))) {
        for (size_t i = 0; i < animations.size(); i++) {
            bool selected = current == i;
            if (ImGui::Selectable(animations[i].c_str(), selected))
                model.setCurrentAnimation(i);
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndChild();
    }

    ImGui::End();
    ImGui::Render();
}

void Engine::drawModels(bool isFramebuffer, double timeInSeconds)
{
    if (isFramebuffer)
        framebuffer.bind();
    else
        framebuffer.unbind();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glViewport(isFramebuffer ? 0 :  sidePanelWidth, 0, viewport.x, viewport.y);

    shader.use();
    shader.set<int>("isFramebuffer", isFramebuffer);
    MVPTransforms transforms = camera.getMVPTransforms(viewport.x, viewport.y);
    shader.writeBuffer("mvp", &transforms, 0, sizeof(MVPTransforms));

    for (unsigned int i = 0; i < models.size(); i++) {
        Model& model = models[i];
        if (model.isCalled("floor")) {
            model.setSize(glm::vec3(10.0, 0.5, 10.0), false);
            model.setPosition(glm::vec3(0.0, -0.5, 0.0));
        }
        if (model.isCalled("player")) {
            model.setSize(glm::vec3(0.0, 1.0, 0.0), true);
            model.setPosition(glm::vec3(0.0, 0.0, 0.0));
        }
        shader.set<unsigned int>("modelId", i + 1);
        model.draw(shader, timeInSeconds);
    }
}

void Engine::draw(float timeInSeconds)
{
    drawModels(true, timeInSeconds);
    drawModels(false, timeInSeconds);
}
