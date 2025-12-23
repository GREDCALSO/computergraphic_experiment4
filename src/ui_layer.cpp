#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "ui_layer.h"

#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <string>

#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include"Auth.h"

namespace {
    std::string OpenTextureFileDialog() {
        char fileBuffer[MAX_PATH] = { 0 };
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.hdr\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = fileBuffer;
        ofn.nMaxFile = MAX_PATH;
        std::filesystem::path initial = std::filesystem::current_path() / "resources";
        std::string initialStr = initial.string();
        ofn.lpstrInitialDir = initialStr.c_str();
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
        if (GetOpenFileNameA(&ofn) == TRUE) {
            return std::string(fileBuffer);
        }
        return {};
    }
}

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
        const float inspectorTop = 280.0f; // below light panel
        ImGui::SetNextWindowPos(ImVec2(xPos, inspectorTop));
        ImGui::SetNextWindowSize(ImVec2(sidebarWidth, io.DisplaySize.y - inspectorTop));
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

                    // Color
                    ImGui::Separator();
                    ImGui::Text("Color");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##color")) {
                        editable->color = scene.getDefaultColor(editable->type);
                        editable->matDiffuse = editable->color;
                        editable->matAmbient = editable->color * 0.2f;
                    }
                    if (ImGui::ColorEdit3("##color", reinterpret_cast<float*>(&editable->color))) {
                        editable->matDiffuse = editable->color;
                    }

                    ImGui::Separator();
                    ImGui::Text("Material");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##mat")) {
                        glm::vec3 amb, diff, spec;
                        float shin = 32.0f;
                        float ambStr = 1.0f, diffStr = 1.0f, specStr = 1.0f;
                        scene.getDefaultMaterial(amb, diff, spec, shin, ambStr, diffStr, specStr);
                        diff = editable->color;
                        amb = diff * 0.2f;
                        editable->matAmbient = amb;
                        editable->matDiffuse = diff;
                        editable->matSpecular = spec;
                        editable->matShininess = shin;
                        editable->matAmbientStrength = ambStr;
                        editable->matDiffuseStrength = diffStr;
                        editable->matSpecularStrength = specStr;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Metal")) {
                        editable->matAmbient = editable->color * 0.1f;
                        editable->matDiffuse = editable->color * 0.6f;
                        editable->matSpecular = glm::vec3(0.95f);
                        editable->matAmbientStrength = 0.6f;
                        editable->matDiffuseStrength = 0.9f;
                        editable->matSpecularStrength = 1.5f;
                        editable->matShininess = 96.0f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Plastic")) {
                        editable->matAmbient = editable->color * 0.2f;
                        editable->matDiffuse = editable->color;
                        editable->matSpecular = glm::vec3(0.5f);
                        editable->matAmbientStrength = 0.8f;
                        editable->matDiffuseStrength = 1.0f;
                        editable->matSpecularStrength = 0.9f;
                        editable->matShininess = 48.0f;
                    }
                    if (ImGui::Button("Rubber")) {
                        editable->matAmbient = editable->color * 0.4f;
                        editable->matDiffuse = editable->color * 0.6f;
                        editable->matSpecular = glm::vec3(0.1f);
                        editable->matAmbientStrength = 1.2f;
                        editable->matDiffuseStrength = 0.8f;
                        editable->matSpecularStrength = 0.2f;
                        editable->matShininess = 8.0f;
                    }
                    if (ImGui::Button("Default")) {
                        glm::vec3 amb, diff, spec;
                        float shin = 32.0f;
                        float ambStr = 1.0f, diffStr = 1.0f, specStr = 1.0f;
                        scene.getDefaultMaterial(amb, diff, spec, shin, ambStr, diffStr, specStr);
                        diff = editable->color;
                        amb = diff * 0.2f;
                        editable->matAmbient = amb;
                        editable->matDiffuse = diff;
                        editable->matSpecular = spec;
                        editable->matShininess = shin;
                        editable->matAmbientStrength = ambStr;
                        editable->matDiffuseStrength = diffStr;
                        editable->matSpecularStrength = specStr;
                    }
                    ImGui::ColorEdit3("Ambient", reinterpret_cast<float*>(&editable->matAmbient));
                    ImGui::SliderFloat("Ambient Strength", &editable->matAmbientStrength, 0.0f, 2.0f, "%.2f");
                    ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&editable->matDiffuse));
                    ImGui::SliderFloat("Diffuse Strength", &editable->matDiffuseStrength, 0.0f, 2.0f, "%.2f");
                    ImGui::ColorEdit3("Specular", reinterpret_cast<float*>(&editable->matSpecular));
                    ImGui::SliderFloat("Specular Strength", &editable->matSpecularStrength, 0.0f, 2.0f, "%.2f");
                    ImGui::SliderFloat("Shininess", &editable->matShininess, 1.0f, 256.0f, "%.0f");

                    ImGui::Separator();
                    ImGui::Text("Texture");
                    ImGui::SameLine();
                    if (ImGui::Button("Load Texture")) {
                        const std::string path = OpenTextureFileDialog();
                        if (!path.empty()) {
                            if (scene.loadTextureForSelected(path)) {
                                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "%s loaded", std::filesystem::path(path).filename().string().c_str());
                            }
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Remove Texture")) {
                        scene.removeTextureFromSelected();
                    }
                    if (editable->hasTexture) {
                        ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "%s loaded", editable->textureName.c_str());
                    }
                    else {
                        ImGui::TextDisabled("No texture");
                    }

                    int wrapIdx = static_cast<int>(editable->wrapMode);
                    const char* wrapItems[] = { "Repeat", "Clamp to Edge", "Mirrored Repeat" };
                    if (ImGui::Combo("Wrap Mode", &wrapIdx, wrapItems, IM_ARRAYSIZE(wrapItems))) {
                        editable->wrapMode = static_cast<TextureWrapMode>(wrapIdx);
                        scene.applyTextureSettings(*editable);
                    }

                    int filterIdx = static_cast<int>(editable->filterMode);
                    const char* filterItems[] = { "Nearest", "Linear" };
                    if (ImGui::Combo("Filter Mode", &filterIdx, filterItems, IM_ARRAYSIZE(filterItems))) {
                        editable->filterMode = static_cast<TextureFilterMode>(filterIdx);
                        scene.applyTextureSettings(*editable);
                    }

                    int projIdx = static_cast<int>(editable->projection);
                    const char* projItems[] = { "Planar", "Triplanar", "Spherical", "Cylindrical", "Cube" };
                    if (ImGui::Combo("Projection", &projIdx, projItems, IM_ARRAYSIZE(projItems))) {
                        editable->projection = static_cast<TextureProjection>(projIdx);
                    }

                    if (editable->projection == TextureProjection::Planar) {
                        int axisIdx = static_cast<int>(editable->planarAxis);
                        const char* axisItems[] = { "Normal X", "Normal Y", "Normal Z" };
                        if (ImGui::Combo("Planar Axis", &axisIdx, axisItems, IM_ARRAYSIZE(axisItems))) {
                            editable->planarAxis = static_cast<PlanarAxis>(axisIdx);
                        }
                    }

                    ImGui::InputFloat2("UV Scale", reinterpret_cast<float*>(&editable->uvScale), "%.3f");
                    ImGui::SliderFloat2("UV Scale Slider", reinterpret_cast<float*>(&editable->uvScale), 0.1f, 8.0f, "%.2f");

                    ImGui::Separator();
                    if (ImGui::Button("Delete Entity")) {
                        scene.removeSelected();
                    }

                    ImGui::Dummy(ImVec2(0, 12));
                }
            }
        }
        ImGui::End();
    }

    // light panel (independent)
    const float lightWidth = 320.0f;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - lightWidth - 12.0f, 12.0f));
    ImGui::SetNextWindowSize(ImVec2(lightWidth, 260.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowBgAlpha(0.9f);
    if (ImGui::Begin("Light", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse)) {
        LightSettings& light = scene.getLightSettings();

        ImGui::Text("Light Controls");
        ImGui::Separator();

        ImGui::Text("Position");
        ImGui::SameLine();
        if (ImGui::Button("Reset##lightpos")) {
            light.position = glm::vec3(-2.0f, 4.0f, 2.0f);
        }
        float lpos[3] = { light.position.x, light.position.y, light.position.z };
        if (ImGui::InputFloat3("##lightpos", lpos, "%.3f")) {
            light.position = glm::vec3(lpos[0], lpos[1], lpos[2]);
        }

        ImGui::Text("Color");
        ImGui::SameLine();
        if (ImGui::Button("Reset##lightcol")) {
            light.color = glm::vec3(1.0f);
        }
        ImGui::ColorEdit3("##lightcolor", reinterpret_cast<float*>(&light.color));

        ImGui::Separator();
        ImGui::Text("Intensities");
        const float minI = 0.0f, maxI = 2.0f;
        ImGui::SliderFloat("Ambient", &light.ambient, minI, maxI, "%.2f");
        ImGui::SliderFloat("Diffuse", &light.diffuse, minI, maxI, "%.2f");
        ImGui::SliderFloat("Specular", &light.specular, minI, maxI, "%.2f");
        ImGui::SliderFloat("Shininess", &light.shininess, 1.0f, 128.0f, "%.0f");
        if (ImGui::Button("Reset##light")) {
            light.ambient = 0.15f;
            light.diffuse = 0.75f;
            light.specular = 0.25f;
            light.shininess = 32.0f;
        }
    }
    ImGui::End();

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
