#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "axes.h"
#include "camera.h"
#include "grid.h"
#include "hud.h"

namespace {
    int gScreenWidth = 1280;
    int gScreenHeight = 720;
    Camera gCamera;
    bool gRightMouseDown = false;
    bool gFirstDrag = true;
    double gLastX = 0.0;
    double gLastY = 0.0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    gScreenWidth = width;
    gScreenHeight = height;
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    if (gRightMouseDown) {
        gCamera.AdjustSpeed(static_cast<float>(yoffset));
        HudRenderer* hud = reinterpret_cast<HudRenderer*>(glfwGetWindowUserPointer(window));
        if (hud) {
            hud->showSpeed(gCamera.GetSpeed());
        }
    }
    else {
        gCamera.Dolly(static_cast<float>(yoffset));
        HudRenderer* hud = reinterpret_cast<HudRenderer*>(glfwGetWindowUserPointer(window));
        if (hud) {
            hud->showDolly(static_cast<float>(yoffset));
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!gRightMouseDown) {
        return;
    }

    if (gFirstDrag) {
        gLastX = xpos;
        gLastY = ypos;
        gFirstDrag = false;
        return;
    }

    const double xoffset = xpos - gLastX;
    const double yoffset = gLastY - ypos;
    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            gRightMouseDown = true;
            gFirstDrag = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &gLastX, &gLastY);
        }
        else if (action == GLFW_RELEASE) {
            gRightMouseDown = false;
            gFirstDrag = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (!gRightMouseDown) {
        return;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(Camera::MoveDir::Forward, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(Camera::MoveDir::Backward, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(Camera::MoveDir::Left, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(Camera::MoveDir::Right, deltaTime);
    }
}

int main() {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(gScreenWidth, gScreenHeight, "CG Experiment 4", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, gScreenWidth, gScreenHeight);

    AxesRenderer axes;
    axes.init();

    GridRenderer grid;
    grid.init(20, 1.0f);

    HudRenderer hud;
    hud.init();
    glfwSetWindowUserPointer(window, &hud);

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        hud.updateTimers(deltaTime);

        processInput(window, deltaTime);

        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = static_cast<float>(gScreenWidth) / static_cast<float>(gScreenHeight);
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        const glm::mat4 view = gCamera.GetViewMatrix();

        grid.draw(view, projection);
        axes.draw(view, projection);

        hud.draw(gScreenWidth, gScreenHeight);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}