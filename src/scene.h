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
    glm::vec3 color;
    glm::vec3 matAmbient;
    glm::vec3 matDiffuse;
    glm::vec3 matSpecular;
    float matShininess;
};

struct LightSettings {
    glm::vec3 position = glm::vec3(-2.0f, 4.0f, 2.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float ambient = 0.15f;
    float diffuse = 0.75f;
    float specular = 0.25f;
    float shininess = 32.0f;
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

    glm::vec3 getDefaultColor(PrimitiveType type) const { return colorForType(type); }
    void getDefaultMaterial(glm::vec3& ambient, glm::vec3& diffuse, glm::vec3& specular, float& shininess) const;

    LightSettings& getLightSettings() { return light; }
    const LightSettings& getLightSettings() const { return light; }

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
    LightSettings light;
};
