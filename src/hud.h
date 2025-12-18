#pragma once

#include <glm/glm.hpp>

#include "shader.h"

class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer();

    void init();
    void updateTimers(float dt);
    void showSpeed(float speed);
    void showDolly(float delta);
    void draw(int screenWidth, int screenHeight);

private:
    void drawBar(float x, float y, float width, float height, const glm::vec3& color, float fill, int screenWidth, int screenHeight);

    unsigned int VAO = 0;
    unsigned int VBO = 0;
    Shader hudShader;
    bool initialized = false;

    float speedValue = 0.0f;
    float speedTimer = 0.0f;
    float dollyValue = 0.0f;
    float dollyTimer = 0.0f;
};
