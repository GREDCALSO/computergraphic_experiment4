#pragma once

#include <glm/glm.hpp>

#include "shader.h"

class GridRenderer {
public:
    GridRenderer();
    ~GridRenderer();

    void init(int halfExtent = 10, float spacing = 1.0f);
    void draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int lineCount = 0;
    Shader gridShader;
    bool initialized = false;
};
