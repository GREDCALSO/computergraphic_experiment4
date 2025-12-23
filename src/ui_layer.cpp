#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "ui_layer.h"

#include <cstdio>
#include <algorithm>

#include"Auth.h"

UiLayer::UiLayer() = default;
UiLayer::~UiLayer() {
    shutdown();
}

void UiLayer::init(GLFWwindow* window) {
    if (initialized) {
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    applyStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    initialized = true;
}

void UiLayer::beginFrame() {
    if (!initialized) {
        return;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UiLayer::draw(SceneRenderer& scene, const Camera& camera) {
    if (!initialized) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // transform panel (top-left)
    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f));
    ImGui::SetNextWindowBgAlpha(0.85f);
    if (ImGui::Begin("Transform", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Transform Mode:");
        ImGui::SameLine();
        ImGui::RadioButton("Select Object", reinterpret_cast<int*>(&mode), static_cast<int>(TransformMode::Select));
        ImGui::SameLine();
        ImGui::RadioButton("Translate", reinterpret_cast<int*>(&mode), static_cast<int>(TransformMode::Translate));
        ImGui::SameLine();
        ImGui::RadioButton("Rotate", reinterpret_cast<int*>(&mode), static_cast<int>(TransformMode::Rotate));
        ImGui::SameLine();
        ImGui::RadioButton("Scale", reinterpret_cast<int*>(&mode), static_cast<int>(TransformMode::Scale));

        const auto& instances = scene.getInstances();
        const int selected = scene.getSelectedIndex();
        if (instances.empty()) {
            ImGui::TextDisabled("No primitives");
        }
        else {
            for (size_t i = 0; i < instances.size(); ++i) {
                char label[64];
                snprintf(label, sizeof(label), "%zu: %s", i, typeLabel(instances[i].type));
                if (ImGui::Selectable(label, static_cast<int>(i) == selected)) {
                    scene.select(static_cast<int>(i));
                }
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("R/F/T/G/Y/H Transform Axis\nDouble-click left button to select/deselect, drag to pan\nMouse wheel adjusts depth");
    }
    ImGui::End();

    // speed hint at top center
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 8.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.2f);
    if (ImGui::Begin("SpeedHint", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Viewing Angle Movement Speed(Keyboard): %.2f", cameraSpeed);
    }
    ImGui::End();

    // inspector panel (right)
    const bool hasSelection = scene.getSelectedIndex() >= 0;
    const float target = hasSelection ? 1.0f : 0.0f;
    const float dt = io.DeltaTime;
    const float speed = 6.0f;
    const float alpha = std::clamp(dt * speed, 0.0f, 1.0f);
    inspectorProgress = inspectorProgress + (target - inspectorProgress) * alpha;

    const float sidebarWidth = 340.0f;
    if (inspectorProgress > 0.01f) {
        const float xPos = io.DisplaySize.x - sidebarWidth * inspectorProgress;
        ImGui::SetNextWindowPos(ImVec2(xPos, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(sidebarWidth, io.DisplaySize.y));
        ImGui::SetNextWindowBgAlpha(0.92f);

        if (ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            const PrimitiveInstance* inst = scene.getSelected();
            if (inst) {
                ImGui::Text("Entity Properties");
                ImGui::Separator();
                ImGui::Text("Type: %s", typeLabel(inst->type));

                PrimitiveInstance* editable = scene.getSelectedMutable();
                if (editable) {
                    // Position
                    ImGui::Separator();
                    ImGui::Text("Position (X Y Z)");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##pos")) {
                        editable->position = glm::vec3(0.0f);
                    }
                    float pos[3] = { editable->position.x, editable->position.y, editable->position.z };
                    if (ImGui::InputFloat3("##pos", pos, "%.3f")) {
                        editable->position = glm::vec3(pos[0], pos[1], pos[2]);
                    }

                    // Rotation
                    ImGui::Separator();
                    ImGui::Text("Rotation (Degrees)");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##rot")) {
                        editable->rotation = glm::vec3(0.0f);
                    }
                    float rot[3] = { editable->rotation.x, editable->rotation.y, editable->rotation.z };
                    if (ImGui::InputFloat3("##rot", rot, "%.2f")) {
                        editable->rotation = glm::vec3(rot[0], rot[1], rot[2]);
                    }

                    // Scale
                    ImGui::Separator();
                    ImGui::Text("Scale (Multiplier)");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##scl")) {
                        editable->scale = glm::vec3(1.0f);
                        if (editable->type == PrimitiveType::Plane) { editable->scale.y = 1.0f; }
                    }
                    float scl[3] = { editable->scale.x, editable->scale.y, editable->scale.z };
                    if (ImGui::InputFloat3("##scl", scl, "%.3f")) {
                        glm::vec3 newScale(scl[0], scl[1], scl[2]);
                        if (editable->type == PrimitiveType::Plane) {
                            newScale.y = 1.0f; // keep plane height locked
                        }
                        newScale = glm::max(newScale, glm::vec3(0.1f));
                        editable->scale = newScale;
                    }

                    ImGui::Dummy(ImVec2(0, 12));
                    ImGui::Separator();
                    ImGui::Text("Lighting");

                    LightSettings& light = scene.getLightSettings();

                    ImGui::Text("Light Position");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##lightpos")) {
                        light.position = glm::vec3(-2.0f, 4.0f, 2.0f);
                    }
                    float lpos[3] = { light.position.x, light.position.y, light.position.z };
                    if (ImGui::InputFloat3("##lightpos", lpos, "%.3f")) {
                        light.position = glm::vec3(lpos[0], lpos[1], lpos[2]);
                    }

                    ImGui::Separator();
                    ImGui::Text("Light Intensities");
                    const float minI = 0.0f, maxI = 2.0f;
                    if (ImGui::SliderFloat("Ambient", &light.ambient, minI, maxI, "%.2f")) {}
                    if (ImGui::SliderFloat("Diffuse", &light.diffuse, minI, maxI, "%.2f")) {}
                    if (ImGui::SliderFloat("Specular", &light.specular, minI, maxI, "%.2f")) {}
                    if (ImGui::SliderFloat("Shininess", &light.shininess, 1.0f, 128.0f, "%.0f")) {}
                    if (ImGui::Button("Reset##light")) {
                        light.ambient = 0.15f;
                        light.diffuse = 0.75f;
                        light.specular = 0.25f;
                        light.shininess = 32.0f;
                    }
                }
            }
        }
        ImGui::End();
    }

    const float barHeight = 64.0f;
    ImGui::SetNextWindowPos(ImVec2(0.0f, io.DisplaySize.y - barHeight));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, barHeight));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("BottomBar", nullptr, flags)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 10.0f));

        ImGui::Text("Scene Primitives: %zu", scene.instanceCount());
        ImGui::SameLine();

        if (ImGui::Button("Generate Primitive")) {
            ImGui::OpenPopup("primitive_popup");
        }

        if (ImGui::BeginPopup("primitive_popup")) {
            const glm::vec3 spawnPos = camera.GetPosition() + camera.GetFront() * 4.0f;

            if (ImGui::MenuItem("Sphere")) {
                scene.addPrimitive(PrimitiveType::Sphere, spawnPos);
            }
            if (ImGui::MenuItem("Cylinder")) {
                scene.addPrimitive(PrimitiveType::Cylinder, spawnPos);
            }
            if (ImGui::MenuItem("Plane")) {
                scene.addPrimitive(PrimitiveType::Plane, spawnPos);
            }
            if (ImGui::MenuItem("Cube")) {
                scene.addPrimitive(PrimitiveType::Cube, spawnPos);
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear All Primitives")) {
            scene.clear();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Use the bottom bar buttons to generate or clear primitives");

        ImGui::PopStyleVar(2);
    }

    ImGui::End();
}

void UiLayer::render() {
    if (!initialized) {
        return;
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UiLayer::shutdown() {
    if (!initialized) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

bool UiLayer::WantCaptureMouse() const {
    if (!initialized) {
        return false;
    }
    return ImGui::GetIO().WantCaptureMouse;
}

bool UiLayer::WantCaptureKeyboard() const {
    if (!initialized) {
        return false;
    }
    return ImGui::GetIO().WantCaptureKeyboard;
}

void UiLayer::applyStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.ScrollbarSize = 12.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.10f, 0.13f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.21f, 0.50f, 0.78f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.45f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.20f, 0.26f, 1.00f);
}

const char* UiLayer::typeLabel(PrimitiveType type) const {
    switch (type) {
    case PrimitiveType::Cube: return "Cube";
    case PrimitiveType::Sphere: return "Sphere";
    case PrimitiveType::Cylinder: return "Cylinder";
    case PrimitiveType::Plane: return "Plane";
    }
    return "Unknown";
}
