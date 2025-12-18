#pragma once

#include <glm/glm.hpp>

#include "shader.h"

class AxesRenderer {
public:
    AxesRenderer();
    ~AxesRenderer();

    void init();
    void draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    Shader axisShader;
    bool initialized = false;
};
