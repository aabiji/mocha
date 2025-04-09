#include <imgui.h>

#define GLAD_GL_IMPLEMENTATION 1
#include <glad.h>

#include "engine.h"

void Engine::init(glm::vec4 view)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    viewport = view;
    glViewport(viewport.x, viewport.y, viewport.z, viewport.w);

    camera.init(glm::vec3(0.0), 3);

    shader.load(GL_VERTEX_SHADER, "../shaders/vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../shaders/fragment.glsl");
    shader.assemble();
    shader.use();

    initLights();

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
    bool xInside = mouseX >= viewport.x && mouseX <= viewport.x + viewport.z;
    bool yInside = mouseY >= viewport.y && mouseY <= viewport.y + viewport.w;
    return xInside && yInside;
}

void Engine::resizeViewport(int width, int height)
{
    viewport.z = width;
    viewport.w = height;
    glViewport(viewport.x, viewport.y, viewport.z, viewport.w);
}

void Engine::mouseHover(int mouseX, int mouseY)
{
    (void)mouseX;
    (void)mouseY;
    /*
    // Convert mouse x and y into normalized device coordinates
    float x = (2.0 * mouseX) / viewport.z - 1.0;
    float y = 1.0 - (2.0 * mouseY) / viewport.w;

    // Convert the normalize device coordinate to a point in clip space
    // We're setting z to -1 to make it point inside the window
    glm::vec4 rayClip = glm::vec4(x, y, -1, 1.0);

    // Convert the coordinate from clip space to view space
    glm::vec4 rayView = glm::inverse(projectionMatrix) * rayClip;
    rayView = glm::vec4(rayView.x, rayView.y, -1.0, 1.0);

    // Convert the coordinate from view space to world space
    glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayView;
    glm::vec3 direction = glm::normalize(glm::vec3(rayWorld));

    for (Model& model : models) {
        if (model.box.rayIntersects(camera.getPosition(), direction))
            std::cout << "Mouse hovering over " << model.name << "\n";
    }
    */
}

void Engine::loadModel(std::string name, std::string path, std::string base)
{
    pool.dispatch([&, name, path, base] {
        Model model(name, path, base);
        models.push_back(model);
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
    ImGui::SetWindowPos(ImVec2(viewport.x, viewport.y));
    ImGui::SetWindowSize(ImVec2(viewport.x, viewport.w));

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
    shader.set<glm::mat4>("projection", camera.getProjection(viewport.z, viewport.w));
    shader.set<glm::vec3>("viewPosition", camera.getPosition());

    for (Model& model : models)
        model.draw(shader, timeInSeconds);
}
