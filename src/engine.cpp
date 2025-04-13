#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

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

    shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
    shader.assemble();
    shader.use();

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
    shader.cleanup();
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
    glViewport(sidePanelWidth, 0, viewport.x, viewport.y);
}

#include <iostream>
void Engine::handleClick(int mouseX, int mouseY)
{
    glm::vec3 rayOrigin;
    glm::vec3 rayDirection;
    screenPosToWorldRay(mouseX - sidePanelWidth, mouseY, rayOrigin, rayDirection);

    for (Model& model : models) {
        if (model.intersects(rayOrigin, rayDirection))
            std::cout << model.name << "\n";
    }
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

    unsigned int lightsBuffer = shader.createBuffer(1, sizeof(lights));
    shader.updateBuffer(lightsBuffer, sizeof(lights), &lights);
}

// Convert a coordinate to a ray in world space with an origin and direction
// (x, y) is an on screen position, where (0, 0) is at the top left
void Engine::screenPosToWorldRay(
    float x, float y, glm::vec3& origin, glm::vec3& direction
)
{
    // Get the normalized device coordinates on the near and far plane
    glm::vec4 nearPos = glm::vec4(
        (2.0 * x) / viewport.x - 1.0f,
        1.0f - (2.0 * y) / viewport.y,
        -1.0, 1.0
    );
    glm::vec4 farPos = glm::vec4(
        (2.0 * x) / viewport.x - 1.0f,
        1.0f - (2.0 * y) / viewport.y,
        0.0, 1.0
    );

    // This matrix converts a position from normalized device
    // coordinate space into view space and from view space
    // into world space
    glm::mat4 inverse = glm::inverse(
        camera.getProjection(viewport.x, viewport.y) * camera.getView()
    );

    // Compute the ray start and end positions in world space
    glm::vec4 rayStart = inverse * nearPos;
    glm::vec4 rayEnd = inverse * farPos;
    rayStart /= rayStart.w;
    rayEnd /= rayEnd.w;

    origin = rayStart;
    direction = glm::normalize(rayEnd - rayStart);
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


void Engine::draw(float timeInSeconds)
{
    shader.use();
    shader.set<glm::mat4>("view", camera.getView());
    shader.set<glm::mat4>(
        "projection", camera.getProjection(viewport.x, viewport.y));
    shader.set<glm::vec3>("viewPosition", camera.getPosition());

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
