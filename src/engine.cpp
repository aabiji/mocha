#define GLAD_GL_IMPLEMENTATION 1
#include <imgui.h>
#include "engine.h"
#include "log.h"

void Engine::init(int width, int height, int panelSize)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    sidePanelWidth = panelSize;
    resizeViewport(width, height);

    frameBuffer.init(windowSize.x, windowSize.y);

    frameBufferShader.load(GL_VERTEX_SHADER, "../shaders/framebuffer_vertex.glsl");
    frameBufferShader.load(GL_FRAGMENT_SHADER, "../shaders/framebuffer_fragment.glsl");
    frameBufferShader.assemble();

    defaultShader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
    defaultShader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
    defaultShader.assemble();
    defaultShader.use();

    initLights();
    camera.init(glm::vec3(0.0), 3);

    pool.init(3);
    loadModel("floor", "../assets/cube.fbx", "");
    loadModel("player", "../assets/characters/Barbarian.fbx", "../assets/characters/");
}

void Engine::cleanup()
{
    for (Model& model : models)
        model.cleanup();
    frameBuffer.cleanup();
    frameBufferShader.cleanup();
    defaultShader.cleanup();
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

void Engine::handleMouseHover(int mouseX, int mouseY)
{
    std::cout << frameBuffer.readPixel(mouseX, mouseY) << "\n";
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

    unsigned int lightsBuffer = defaultShader.createBuffer(1, sizeof(lights));
    defaultShader.updateBuffer(lightsBuffer, sizeof(lights), &lights);
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
    if (iterator == models.end()) { // Model not found
        ImGui::End();
        ImGui::Render();
        return;
    }

    Model& model = *iterator;
    auto animations = model.animationNames();
    size_t current = model.getCurrentAnimation();
    if (animations.size() == 0) { // No animations to play
        ImGui::End();
        ImGui::Render();
        return;
    }

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

void Engine::drawModels(Shader& shader, double timeInSeconds)
{
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
        shader.set<unsigned int>("modelId", i);
        model.draw(shader, timeInSeconds);
    }
}

void Engine::draw(float timeInSeconds)
{
    // Draw to the framebuffer
    frameBuffer.bind();
    frameBufferShader.use();
    frameBufferShader.set<glm::mat4>("view", camera.getView());
    frameBufferShader.set<glm::mat4>("projection", camera.getProjection(windowSize.x, windowSize.y));
    frameBufferShader.set<glm::vec3>("viewPosition", camera.getPosition());
    drawModels(frameBufferShader, timeInSeconds);

    // Draw to the default framebuffer
    frameBuffer.unbind();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    defaultShader.use();
    defaultShader.set<glm::mat4>("view", camera.getView());
    defaultShader.set<glm::mat4>("projection", camera.getProjection(windowSize.x, windowSize.y));
    defaultShader.set<glm::vec3>("viewPosition", camera.getPosition());
    drawModels(defaultShader, timeInSeconds);
}
