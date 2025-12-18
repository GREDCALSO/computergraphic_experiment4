#include "hud.h"

#include <glad/glad.h>

#include <algorithm>
#include <glm/common.hpp>

HudRenderer::HudRenderer() = default;
HudRenderer::~HudRenderer() {
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
}

void HudRenderer::init() {
    if (initialized) {
        return;
    }

    const char* vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";

    hudShader = Shader(vertexShader, fragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized = true;
}

void HudRenderer::updateTimers(float dt) {
    speedTimer = speedTimer > 0.0f ? speedTimer - dt : 0.0f;
    dollyTimer = dollyTimer > 0.0f ? dollyTimer - dt : 0.0f;
}

void HudRenderer::showSpeed(float speed) {
    speedValue = speed;
    speedTimer = 2.0f;
}

void HudRenderer::showDolly(float delta) {
    dollyValue = delta;
    dollyTimer = 1.5f;
}

void HudRenderer::draw(int screenWidth, int screenHeight) {
    if (!initialized) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    hudShader.use();
    glBindVertexArray(VAO);

    if (speedTimer > 0.0f) {
        const float barWidth = 200.0f;
        const float barHeight = 12.0f;
        const float x = 20.0f;
        const float y = 20.0f;
        const float fill = glm::clamp(speedValue / 10.0f, 0.0f, 1.0f);
        drawBar(x, y, barWidth, barHeight, glm::vec3(0.3f, 0.3f, 0.35f), 1.0f, screenWidth, screenHeight);
        drawBar(x, y, barWidth * fill, barHeight, glm::vec3(0.15f, 0.75f, 0.3f), 1.0f, screenWidth, screenHeight);
    }

    if (dollyTimer > 0.0f) {
        const float barWidth = 200.0f;
        const float barHeight = 12.0f;
        const float x = 20.0f;
        const float y = 40.0f;
        const float magnitude = glm::clamp(std::abs(dollyValue) / 5.0f, 0.0f, 1.0f);
        const glm::vec3 color = dollyValue >= 0.0f ? glm::vec3(0.2f, 0.6f, 1.0f) : glm::vec3(0.95f, 0.45f, 0.25f);
        drawBar(x, y, barWidth, barHeight, glm::vec3(0.3f, 0.3f, 0.35f), 1.0f, screenWidth, screenHeight);
        drawBar(x, y, barWidth * magnitude, barHeight, color, 1.0f, screenWidth, screenHeight);
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void HudRenderer::drawBar(float x, float y, float width, float height, const glm::vec3& color, float fill, int screenWidth, int screenHeight) {
    const float x0 = (x / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
    const float y0 = 1.0f - (y / static_cast<float>(screenHeight)) * 2.0f;
    const float x1 = ((x + width * fill) / static_cast<float>(screenWidth)) * 2.0f - 1.0f;
    const float y1 = 1.0f - ((y + height) / static_cast<float>(screenHeight)) * 2.0f;

    const float vertices[] = {
        x0, y0,
        x1, y0,
        x1, y1,
        x0, y0,
        x1, y1,
        x0, y1,
    };

    hudShader.setVec3("color", color);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
