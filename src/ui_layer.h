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

private:
    void applyStyle();

    bool initialized = false;
};
