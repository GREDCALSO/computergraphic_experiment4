#include "grid.h"

#include <glad/glad.h>

#include <vector>

GridRenderer::GridRenderer() = default;

GridRenderer::~GridRenderer() {
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
}

void GridRenderer::init(int halfExtent, float spacing) {
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

    gridShader = Shader(vertexShader, fragmentShader);

    std::vector<float> vertices;
    const float extent = static_cast<float>(halfExtent) * spacing;
    const glm::vec3 mainColor(0.35f, 0.35f, 0.38f);
    const glm::vec3 axisColor(0.6f, 0.6f, 0.65f);

    for (int i = -halfExtent; i <= halfExtent; ++i) {
        const float coord = static_cast<float>(i) * spacing;
        const glm::vec3 color = (i == 0) ? axisColor : mainColor;

        // Lines parallel to X axis (varying z)
        vertices.push_back(-extent); vertices.push_back(0.0f); vertices.push_back(coord);
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        vertices.push_back(extent); vertices.push_back(0.0f); vertices.push_back(coord);
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);

        // Lines parallel to Z axis (varying x)
        vertices.push_back(coord); vertices.push_back(0.0f); vertices.push_back(-extent);
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
        vertices.push_back(coord); vertices.push_back(0.0f); vertices.push_back(extent);
        vertices.push_back(color.r); vertices.push_back(color.g); vertices.push_back(color.b);
    }

    lineCount = static_cast<int>(vertices.size() / (6 * 2)); // two points per line

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized = true;
}

void GridRenderer::draw(const glm::mat4& view, const glm::mat4& projection) {
    if (!initialized) {
        return;
    }

    gridShader.use();
    const glm::mat4 model(1.0f);
    gridShader.setMat4("model", model);
    gridShader.setMat4("view", view);
    gridShader.setMat4("projection", projection);

    glBindVertexArray(VAO);
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, lineCount * 2);
    glBindVertexArray(0);
}
