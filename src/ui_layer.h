#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "scene.h"
#include "camera.h"

class UiLayer {
public:
    UiLayer();
    ~UiLayer();

    void init(GLFWwindow* window);
    void beginFrame();
    void draw(SceneRenderer& scene, const Camera& camera);
    void render();
    void shutdown();

    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

    enum class TransformMode { Select, Translate, Rotate, Scale };
    TransformMode getMode() const { return mode; }
    void setCameraSpeed(float speed) { cameraSpeed = speed; }

private:
    void applyStyle();
    const char* typeLabel(PrimitiveType type) const;

    bool initialized = false;
    TransformMode mode = TransformMode::Select;
    float cameraSpeed = 0.0f;
};
