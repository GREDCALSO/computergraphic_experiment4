#include "axes.h"

#include <glad/glad.h>

#include <array>

AxesRenderer::AxesRenderer() = default;

AxesRenderer::~AxesRenderer() {
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
}

void AxesRenderer::init() {
    if (initialized) {
        return;
    }

    const char* vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        out vec3 vColor;
        void main() {
            vColor = aColor;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShader = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    axisShader = Shader(vertexShader, fragmentShader);

    constexpr std::array<float, 36> axisVertices = {
        // positions          // colors
         0.0f, 0.0f, 0.0f,     1.0f, 0.0f, 0.0f,
         5.0f, 0.0f, 0.0f,     1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f,     0.0f, 1.0f, 0.0f,
         0.0f, 5.0f, 0.0f,     0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f,     0.0f, 0.0f, 1.0f,
         0.0f, 0.0f, 5.0f,     0.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized = true;
}

void AxesRenderer::draw(const glm::mat4& view, const glm::mat4& projection) {
    if (!initialized) {
        return;
    }

    axisShader.use();
    const glm::mat4 model(1.0f);
    axisShader.setMat4("model", model);
    axisShader.setMat4("view", view);
    axisShader.setMat4("projection", projection);

    glBindVertexArray(VAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}
