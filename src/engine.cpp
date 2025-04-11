#include <imgui.h>

#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

#include "engine.h"
#include "log.h"

void Engine::init(int width, int height, int panelSize)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
    shader.assemble();
    shader.use();

    sidePanelWidth = panelSize;
    resizeViewport(width, height);

    initLights();
    camera.init(glm::vec3(0.0), 3);

    pool.init(3);
    loadModel("floor", "../assets/cube.glb", "");
    loadModel("player", "../assets/characters/Barbarian.fbx", "../assets/characters/");
}

void Engine::cleanup()
{
    for (Model& model : models)
        model.cleanup();
    shader.cleanup();
    pool.terminate();
}

void Engine::rotateCamera(bool left) { camera.rotate(left ? -1 : 1); }
void Engine::zoomCamera(int deltaY) { camera.zoom(deltaY); }

bool Engine::insideViewport(int mouseX, int mouseY)
{
    bool xInside = mouseX >= windowSize.x && mouseX <= windowSize.x + windowSize.x;
    bool yInside = mouseY >= windowSize.y && mouseY <= windowSize.y + windowSize.y;
    return xInside && yInside;
}

void Engine::resizeViewport(int width, int height)
{
    windowSize.y = height;
    windowSize.x = width - sidePanelWidth;
    glViewport(sidePanelWidth, 0, windowSize.x, windowSize.y);
}

void Engine::loadModel(std::string name, std::string path, std::string base)
{
    pool.dispatch([&, name, path, base] {
        try {
            Model model(name, path, base);
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
        glm::vec3(5.0, 1.0, 0.0)
    };

    Light lights[4];
    for (int i = 0; i < 4; i++) {
        lights[i] = {
            .color = glm::vec3(0.8),
            .position = positions[i],
            .c = 1.0, .l = 0.08, .q = 0.032
        };
    }

    unsigned int lightsBuffer = shader.createBuffer(1, sizeof(lights));
    shader.updateBuffer(lightsBuffer, sizeof(lights), &lights);
}

void Engine::drawGUI()
{
    ImGui::NewFrame();

    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    ImGui::Begin("Info", nullptr, flags);

    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImVec2(sidePanelWidth, windowSize.y));

    ImGui::Text("%s", ("FPS: " + std::to_string(fps)).c_str());

    auto search = [](Model& m) { return m.isCalled("player"); };
    auto iterator = std::find_if(models.begin(), models.end(), search);
    if (iterator == models.end()) {
        ImGui::End();
        ImGui::Render();
        return;
    }

    Model& model = *iterator;
    auto animations = model.animationNames();
    size_t current = model.getCurrentAnimation();

    ImGui::Separator();
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

void Engine::draw(float timeInSeconds)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);

    shader.use();
    shader.set<glm::mat4>("view", camera.getView());
    shader.set<glm::mat4>("projection", camera.getProjection(windowSize.x, windowSize.y));
    shader.set<glm::vec3>("viewPosition", camera.getPosition());

    for (Model& model : models) {
        if (model.isCalled("floor")) {
            model.setSize(glm::vec3(10.0, 0.5, 10.0), false);
            model.setPosition(glm::vec3(0.0, -0.5, 0.0));
        }
        if (model.isCalled("player")) {
            model.setSize(glm::vec3(0.0, 1.0, 0.0), true);
            model.setPosition(glm::vec3(0.0, 0.0, 0.0));
        }
        model.draw(shader, timeInSeconds);
    }
}
