#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include "axes.h"
#include "camera.h"
#include "grid.h"
#include "hud.h"
#include "scene.h"
#include "ui_layer.h"
#include <limits>
#include <cmath>

namespace {
    int gScreenWidth = 1920;
    int gScreenHeight = 1080;
    Camera gCamera;
    bool gRightMouseDown = false;
    bool gLeftMouseDown = false;
    bool gDraggingObject = false;
    bool gFirstDrag = true;
    double gLastX = 0.0;
    double gLastY = 0.0;
    double gLastLeftClickTime = 0.0;
    glm::vec3 gDragPlaneNormal(0.0f, 1.0f, 0.0f);
    glm::vec3 gDragPlanePoint(0.0f);
    glm::vec3 gDragOffset(0.0f);

    struct AppContext {
        HudRenderer* hud = nullptr;
        UiLayer* ui = nullptr;
        SceneRenderer* scene = nullptr;
    };
}

glm::vec3 screenRayDirection(double xpos, double ypos) {
    const float x = static_cast<float>(xpos);
    const float y = static_cast<float>(ypos);
    const float ndcX = (2.0f * x) / static_cast<float>(gScreenWidth) - 1.0f;
    const float ndcY = 1.0f - (2.0f * y) / static_cast<float>(gScreenHeight);

    const glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(gScreenWidth) / static_cast<float>(gScreenHeight), 0.1f, 100.0f);
    const glm::mat4 invProj = glm::inverse(projection);
    glm::vec4 eye = invProj * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    eye.z = -1.0f;
    eye.w = 0.0f;

    const glm::mat4 invView = glm::inverse(gCamera.GetViewMatrix());
    const glm::vec4 world = invView * eye;
    return glm::normalize(glm::vec3(world));
}

bool raySphereHit(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& center, float radius, float& tHit) {
    const glm::vec3 oc = rayOrigin - center;
    const float a = glm::dot(rayDir, rayDir);
    const float b = 2.0f * glm::dot(oc, rayDir);
    const float c = glm::dot(oc, oc) - radius * radius;
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }
    const float sqrtDisc = std::sqrt(discriminant);
    const float t0 = (-b - sqrtDisc) / (2.0f * a);
    const float t1 = (-b + sqrtDisc) / (2.0f * a);
    float t = t0 > 0.0f ? t0 : t1;
    if (t <= 0.0f) {
        return false;
    }
    tHit = t;
    return true;
}

int pickInstance(double xpos, double ypos, const SceneRenderer& scene) {
    const auto& instances = scene.getInstances();
    if (instances.empty()) {
        return -1;
    }

    const glm::vec3 rayOrigin = gCamera.GetPosition();
    const glm::vec3 rayDir = screenRayDirection(xpos, ypos);

    float closest = std::numeric_limits<float>::max();
    int best = -1;

    for (size_t i = 0; i < instances.size(); ++i) {
        float baseRadius = 0.8f;
        switch (instances[i].type) {
        case PrimitiveType::Cube: baseRadius = 0.9f; break;
        case PrimitiveType::Sphere: baseRadius = 0.6f; break;
        case PrimitiveType::Cylinder: baseRadius = 0.8f; break;
        case PrimitiveType::Plane: baseRadius = 0.8f; break;
        }
        const float scaleMax = glm::compMax(instances[i].scale);
        const float radius = baseRadius * scaleMax;

        float tHit = 0.0f;
        if (raySphereHit(rayOrigin, rayDir, instances[i].position, radius, tHit)) {
            if (tHit < closest) {
                closest = tHit;
                best = static_cast<int>(i);
            }
        }
    }

    return best;
}

bool rayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planePoint, const glm::vec3& planeNormal, glm::vec3& outPoint) {
    const float denom = glm::dot(rayDir, planeNormal);
    if (std::abs(denom) < 1e-4f) {
        return false;
    }
    const float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    if (t < 0.0f) {
        return false;
    }
    outPoint = rayOrigin + t * rayDir;
    return true;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    gScreenWidth = width;
    gScreenHeight = height;
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    AppContext* ctx = reinterpret_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx && ctx->ui && ctx->ui->WantCaptureMouse()) {
        return;
    }

    if (ctx && ctx->scene && ctx->ui && ctx->ui->getMode() == UiLayer::TransformMode::Translate && ctx->scene->getSelectedIndex() >= 0) {
        const float depthStep = 0.25f * static_cast<float>(yoffset);
        ctx->scene->translateSelected(gCamera.GetFront() * depthStep);
        return;
    }

    if (gRightMouseDown) {
        gCamera.AdjustSpeed(static_cast<float>(yoffset));
        HudRenderer* hud = ctx ? ctx->hud : nullptr;
        if (hud) {
            hud->showSpeed(gCamera.GetSpeed());
        }
    }
    else {
        gCamera.Dolly(static_cast<float>(yoffset));
        HudRenderer* hud = ctx ? ctx->hud : nullptr;
        if (hud) {
            hud->showDolly(static_cast<float>(yoffset));
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    AppContext* ctx = reinterpret_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx && ctx->ui && ctx->ui->WantCaptureMouse()) {
        return;
    }

    if (gDraggingObject && ctx && ctx->scene && ctx->ui && ctx->ui->getMode() == UiLayer::TransformMode::Translate && ctx->scene->getSelectedIndex() >= 0) {
        const glm::vec3 rayOrigin = gCamera.GetPosition();
        const glm::vec3 rayDir = screenRayDirection(xpos, ypos);
        glm::vec3 hit;
        if (rayPlaneIntersection(rayOrigin, rayDir, gDragPlanePoint, gDragPlaneNormal, hit)) {
            ctx->scene->setSelectedPosition(hit + gDragOffset);
        }
    }

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
    AppContext* ctx = reinterpret_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx && ctx->ui && ctx->ui->WantCaptureMouse()) {
        return;
    }

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

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            gLeftMouseDown = true;
            double xpos = 0.0;
            double ypos = 0.0;
            glfwGetCursorPos(window, &xpos, &ypos);
            const double now = glfwGetTime();
            const bool isDoubleClick = (now - gLastLeftClickTime) < 0.25;
            gLastLeftClickTime = now;

            if (ctx && ctx->scene) {
                const int hit = pickInstance(xpos, ypos, *ctx->scene);

                if (isDoubleClick) {
                    if (hit >= 0) {
                        if (ctx->scene->getSelectedIndex() == hit) {
                            ctx->scene->clearSelection();
                            gDraggingObject = false;
                        }
                        else {
                            ctx->scene->select(hit);
                        }
                    }
                    else {
                        ctx->scene->clearSelection();
                        gDraggingObject = false;
                    }
                }
                else {
                    // single click: allow drag if already selected in translate mode
                    if (ctx->ui && ctx->ui->getMode() == UiLayer::TransformMode::Translate && ctx->scene->getSelectedIndex() >= 0) {
                        const PrimitiveInstance* inst = ctx->scene->getSelected();
                        if (inst) {
                            const glm::vec3 rayOrigin = gCamera.GetPosition();
                            const glm::vec3 rayDir = screenRayDirection(xpos, ypos);
                            gDragPlaneNormal = gCamera.GetFront();
                            gDragPlanePoint = inst->position;
                            glm::vec3 hitPoint;
                            if (rayPlaneIntersection(rayOrigin, rayDir, gDragPlanePoint, gDragPlaneNormal, hitPoint)) {
                                gDragOffset = inst->position - hitPoint;
                                gDraggingObject = true;
                            }
                        }
                    }
                }
            }
        }
        else if (action == GLFW_RELEASE) {
            gLeftMouseDown = false;
            gDraggingObject = false;
        }
    }
}

void processInput(GLFWwindow* window, float deltaTime, SceneRenderer& scene, UiLayer& ui) {
    AppContext* ctx = reinterpret_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx && ctx->ui && ctx->ui->WantCaptureKeyboard()) {
        return;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // camera movement only when RMB is held
    if (gRightMouseDown) {
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

    // transforms regardless of RMB
    const int selected = scene.getSelectedIndex();
    const bool hasSelection = selected >= 0;
    const float moveStep = 0.01f;
    const float rotStep = 1.0f;
    const float scaleStep = 0.01f;

    if (hasSelection) {
        switch (ui.getMode()) {
        case UiLayer::TransformMode::Translate:
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) scene.translateSelected(glm::vec3(moveStep, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) scene.translateSelected(glm::vec3(-moveStep, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) scene.translateSelected(glm::vec3(0.0f, moveStep, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) scene.translateSelected(glm::vec3(0.0f, -moveStep, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) scene.translateSelected(glm::vec3(0.0f, 0.0f, moveStep));
            if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) scene.translateSelected(glm::vec3(0.0f, 0.0f, -moveStep));
            break;
        case UiLayer::TransformMode::Rotate:
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) scene.rotateSelected(glm::vec3(rotStep, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) scene.rotateSelected(glm::vec3(-rotStep, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) scene.rotateSelected(glm::vec3(0.0f, rotStep, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) scene.rotateSelected(glm::vec3(0.0f, -rotStep, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) scene.rotateSelected(glm::vec3(0.0f, 0.0f, rotStep));
            if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) scene.rotateSelected(glm::vec3(0.0f, 0.0f, -rotStep));
            break;
        case UiLayer::TransformMode::Scale:
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) scene.scaleSelected(glm::vec3(scaleStep, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) scene.scaleSelected(glm::vec3(-scaleStep, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) scene.scaleSelected(glm::vec3(0.0f, scaleStep, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) scene.scaleSelected(glm::vec3(0.0f, -scaleStep, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) scene.scaleSelected(glm::vec3(0.0f, 0.0f, scaleStep));
            if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) scene.scaleSelected(glm::vec3(0.0f, 0.0f, -scaleStep));
            break;
        case UiLayer::TransformMode::Select:
        default:
            break;
        }
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

    SceneRenderer scene;
    scene.init();

    UiLayer ui;
    ui.init(window);

    AppContext ctx;
    ctx.hud = &hud;
    ctx.ui = &ui;
    ctx.scene = &scene;
    glfwSetWindowUserPointer(window, &ctx);

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        hud.updateTimers(deltaTime);
        ui.setCameraSpeed(gCamera.GetSpeed());
        ui.beginFrame();

        processInput(window, deltaTime, scene, ui);

        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = static_cast<float>(gScreenWidth) / static_cast<float>(gScreenHeight);
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        const glm::mat4 view = gCamera.GetViewMatrix();

        grid.draw(view, projection);
        axes.draw(view, projection);
        scene.draw(view, projection, gCamera.GetPosition());

        hud.draw(gScreenWidth, gScreenHeight);
        ui.draw(scene, gCamera);
        ui.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ui.shutdown();
    glfwTerminate();
    return 0;
}