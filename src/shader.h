#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>

class Shader {
public:
    Shader() = default;
    Shader(const char* vertexSrc, const char* fragmentSrc);

    void use() const;
    GLuint id() const { return programId; }

    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

private:
    GLuint programId = 0;
    GLuint compile(GLenum type, const char* source);
};
