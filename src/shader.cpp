#include "shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>

Shader::Shader(const char* vertexSrc, const char* fragmentSrc) {
    const GLuint vert = compile(GL_VERTEX_SHADER, vertexSrc);
    const GLuint frag = compile(GL_FRAGMENT_SHADER, fragmentSrc);

    programId = glCreateProgram();
    glAttachShader(programId, vert);
    glAttachShader(programId, frag);
    glLinkProgram(programId);

    GLint success = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programId, 512, nullptr, infoLog);
        std::cerr << "Shader link failed: " << infoLog << std::endl;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Shader::use() const {
    glUseProgram(programId);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    const GLint loc = glGetUniformLocation(programId, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    const GLint loc = glGetUniformLocation(programId, name.c_str());
    glUniform2fv(loc, 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    const GLint loc = glGetUniformLocation(programId, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void Shader::setFloat(const std::string& name, float value) const {
    const GLint loc = glGetUniformLocation(programId, name.c_str());
    glUniform1f(loc, value);
}

void Shader::setInt(const std::string& name, int value) const {
    const GLint loc = glGetUniformLocation(programId, name.c_str());
    glUniform1i(loc, value);
}

GLuint Shader::compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compile failed: " << infoLog << std::endl;
    }

    return shader;
}
