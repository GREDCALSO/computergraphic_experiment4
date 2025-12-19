#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <map>
#include <vector>

#include "shader.h"

enum class PrimitiveType {
    Cube,
    Sphere,
    Cylinder,
    Plane
};

struct PrimitiveInstance {
    PrimitiveType type;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation; // Euler degrees XYZ
};

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    void init();
    void addPrimitive(PrimitiveType type, const glm::vec3& position = glm::vec3(0.0f));
    void clear();
    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    size_t instanceCount() const { return instances.size(); }
    const std::vector<PrimitiveInstance>& getInstances() const { return instances; }
    int getSelectedIndex() const { return selectedIndex; }
    void select(int index);
    void clearSelection();
    void translateSelected(const glm::vec3& delta);
    void rotateSelected(const glm::vec3& deltaDegrees);
    void scaleSelected(const glm::vec3& deltaScale);
    void setSelectedPosition(const glm::vec3& position);
    PrimitiveInstance* getSelectedMutable();
    const PrimitiveInstance* getSelected() const;

private:
    struct Mesh {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        GLsizei indexCount = 0;
    };

    Mesh buildCube();
    Mesh buildPlane();
    Mesh buildSphere(int slices = 32, int stacks = 18);
    Mesh buildCylinder(int slices = 32);
    Mesh createMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void destroyMesh(Mesh& mesh);
    void ensureMesh(PrimitiveType type);
    glm::vec3 colorForType(PrimitiveType type) const;

    Shader litShader;
    bool initialized = false;

    std::map<PrimitiveType, Mesh> meshes;
    std::vector<PrimitiveInstance> instances;
    int selectedIndex = -1;
};
