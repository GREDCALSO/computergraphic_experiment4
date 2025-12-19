#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "ui_layer.h"

#include <cstdio>

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
        ImGui::Text("Transform Mode");
        ImGui::SameLine();
        ImGui::RadioButton("Select Objective", reinterpret_cast<int*>(&mode), static_cast<int>(TransformMode::Select));
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
        ImGui::TextDisabled("R/T/Y -> increase\nF/G/H -> decrease");
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
