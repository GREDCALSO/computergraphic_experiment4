#include "scene.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

SceneRenderer::SceneRenderer() = default;

SceneRenderer::~SceneRenderer() {
    for (auto& [_, mesh] : meshes) {
        destroyMesh(mesh);
    }
}

void SceneRenderer::init() {
    if (initialized) {
        return;
    }

    const char* vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 vNormal;
        out vec3 vWorldPos;

        void main() {
            vec4 worldPos = model * vec4(aPos, 1.0);
            vWorldPos = worldPos.xyz;
            vNormal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * worldPos;
        }
    )";

    const char* fragmentShader = R"(
        #version 330 core
        in vec3 vNormal;
        in vec3 vWorldPos;

        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 cameraPos;
        uniform float ambientStrength;
        uniform float diffuseStrength;
        uniform float specularStrength;
        uniform float shininess;
        uniform vec3 matAmbient;
        uniform vec3 matDiffuse;
        uniform vec3 matSpecular;

        out vec4 FragColor;

        void main() {
            vec3 N = normalize(vNormal);
            vec3 L = normalize(lightPos - vWorldPos);
            float diff = max(dot(N, L), 0.0);

            vec3 V = normalize(cameraPos - vWorldPos);
            vec3 H = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0), shininess);

            vec3 ambient = ambientStrength * lightColor * matAmbient;
            vec3 diffuse = diffuseStrength * diff * lightColor * matDiffuse;
            vec3 specular = specularStrength * spec * lightColor * matSpecular;

            FragColor = vec4(ambient + diffuse + specular, 1.0);
        }
    )";

    litShader = Shader(vertexShader, fragmentShader);
    initialized = true;
}

void SceneRenderer::addPrimitive(PrimitiveType type, const glm::vec3& position) {
    ensureMesh(type);
    glm::vec3 amb, diff, spec;
    float shin = 32.0f;
    getDefaultMaterial(amb, diff, spec, shin);
    diff = colorForType(type);
    amb = diff * 0.2f;
    instances.push_back({ type, position, glm::vec3(1.0f), glm::vec3(0.0f), diff, amb, diff, spec, shin });
}

void SceneRenderer::clear() {
    instances.clear();
    selectedIndex = -1;
}

void SceneRenderer::draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    if (!initialized) {
        return;
    }

    litShader.use();
    litShader.setMat4("view", view);
    litShader.setMat4("projection", projection);
    litShader.setVec3("cameraPos", cameraPos);

    litShader.setVec3("lightPos", light.position);
    litShader.setVec3("lightColor", light.color);
    litShader.setFloat("ambientStrength", light.ambient);
    litShader.setFloat("diffuseStrength", light.diffuse);
    litShader.setFloat("specularStrength", light.specular);
    // shininess will be set per-instance

    for (const auto& instance : instances) {
        const auto it = meshes.find(instance.type);
        if (it == meshes.end()) {
            continue;
        }

        glm::mat4 model(1.0f);
        model = glm::translate(model, instance.position);
        model = glm::rotate(model, glm::radians(instance.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(instance.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(instance.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, instance.scale);
        litShader.setMat4("model", model);
        litShader.setVec3("matAmbient", instance.matAmbient);
        litShader.setVec3("matDiffuse", instance.matDiffuse);
        litShader.setVec3("matSpecular", instance.matSpecular);
        litShader.setFloat("shininess", instance.matShininess * light.shininess);

        glBindVertexArray(it->second.VAO);
        glDrawElements(GL_TRIANGLES, it->second.indexCount, GL_UNSIGNED_INT, nullptr);

        const size_t idx = static_cast<size_t>(&instance - instances.data());
        if (static_cast<int>(idx) == selectedIndex) {
            // draw outline in wireframe for selection highlight
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            const glm::vec3 highlight(1.0f, 0.9f, 0.3f);
            litShader.setVec3("matAmbient", highlight * 0.25f);
            litShader.setVec3("matDiffuse", highlight);
            litShader.setVec3("matSpecular", glm::vec3(1.0f));
            litShader.setFloat("shininess", instance.matShininess * light.shininess);
            glDrawElements(GL_TRIANGLES, it->second.indexCount, GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    glBindVertexArray(0);

    // draw light indicator
    ensureMesh(PrimitiveType::Cube);
    const auto itLight = meshes.find(PrimitiveType::Cube);
    if (itLight != meshes.end()) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, light.position);
        model = glm::scale(model, glm::vec3(0.3f));
        litShader.setMat4("model", model);
        litShader.setVec3("matAmbient", light.color * 0.3f);
        litShader.setVec3("matDiffuse", light.color);
        litShader.setVec3("matSpecular", glm::vec3(1.0f));
        litShader.setFloat("shininess", 16.0f);
        glBindVertexArray(itLight->second.VAO);
        glDrawElements(GL_TRIANGLES, itLight->second.indexCount, GL_UNSIGNED_INT, nullptr);
    }
}

SceneRenderer::Mesh SceneRenderer::buildCube() {
    const std::vector<float> vertices = {
        // positions         // normals
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
    };

    const std::vector<unsigned int> indices = {
        0, 1, 2, 2, 3, 0,        // back
        4, 5, 6, 6, 7, 4,        // front
        8, 9,10,10,11, 8,        // left
       12,13,14,14,15,12,        // right
       16,17,18,18,19,16,        // bottom
       20,21,22,22,23,20         // top
    };

    return createMesh(vertices, indices);
}

SceneRenderer::Mesh SceneRenderer::buildPlane() {
    const std::vector<float> vertices = {
        // positions          // normals
        -1.0f, 0.0f, -1.0f,    0.0f, 1.0f, 0.0f,
         1.0f, 0.0f, -1.0f,    0.0f, 1.0f, 0.0f,
         1.0f, 0.0f,  1.0f,    0.0f, 1.0f, 0.0f,
        -1.0f, 0.0f,  1.0f,    0.0f, 1.0f, 0.0f,
    };

    const std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    return createMesh(vertices, indices);
}

SceneRenderer::Mesh SceneRenderer::buildSphere(int slices, int stacks) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int y = 0; y <= stacks; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(stacks);
        const float theta = v * glm::pi<float>();
        const float sinTheta = std::sin(theta);
        const float cosTheta = std::cos(theta);

        for (int x = 0; x <= slices; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(slices);
            const float phi = u * glm::two_pi<float>();
            const float sinPhi = std::sin(phi);
            const float cosPhi = std::cos(phi);

            const float px = cosPhi * sinTheta * 0.5f;
            const float py = cosTheta * 0.5f;
            const float pz = sinPhi * sinTheta * 0.5f;
            const glm::vec3 pos(px, py, pz);
            const glm::vec3 normal = glm::normalize(pos);

            vertices.push_back(pos.x); vertices.push_back(pos.y); vertices.push_back(pos.z);
            vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
        }
    }

    const int stride = slices + 1;
    for (int y = 0; y < stacks; ++y) {
        for (int x = 0; x < slices; ++x) {
            const int i0 = y * stride + x;
            const int i1 = i0 + stride;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i0 + 1);

            indices.push_back(i1);
            indices.push_back(i1 + 1);
            indices.push_back(i0 + 1);
        }
    }

    return createMesh(vertices, indices);
}

SceneRenderer::Mesh SceneRenderer::buildCylinder(int slices) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float halfHeight = 0.5f;
    const int ringSize = slices + 1;

    // side vertices
    for (int i = 0; i < ringSize; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(slices);
        const float angle = t * glm::two_pi<float>();
        const float x = std::cos(angle) * 0.5f;
        const float z = std::sin(angle) * 0.5f;
        const glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        // top ring
        vertices.push_back(x); vertices.push_back(halfHeight); vertices.push_back(z);
        vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
        // bottom ring
        vertices.push_back(x); vertices.push_back(-halfHeight); vertices.push_back(z);
        vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
    }

    // side indices
    for (int i = 0; i < slices; ++i) {
        const int top0 = i * 2;
        const int bot0 = i * 2 + 1;
        const int top1 = (i + 1) * 2;
        const int bot1 = (i + 1) * 2 + 1;

        indices.push_back(top0);
        indices.push_back(bot0);
        indices.push_back(top1);

        indices.push_back(top1);
        indices.push_back(bot0);
        indices.push_back(bot1);
    }

    // top cap ring
    const unsigned int topRingStart = static_cast<unsigned int>(vertices.size() / 6);
    for (int i = 0; i < ringSize; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(slices);
        const float angle = t * glm::two_pi<float>();
        const float x = std::cos(angle) * 0.5f;
        const float z = std::sin(angle) * 0.5f;
        vertices.push_back(x); vertices.push_back(halfHeight); vertices.push_back(z);
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
    }

    const unsigned int topCenterIndex = static_cast<unsigned int>(vertices.size() / 6);
    vertices.push_back(0.0f); vertices.push_back(halfHeight); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);

    for (int i = 0; i < slices; ++i) {
        const unsigned int topCurrent = topRingStart + i;
        const unsigned int topNext = topRingStart + i + 1;
        indices.push_back(topCenterIndex);
        indices.push_back(topCurrent);
        indices.push_back(topNext);
    }

    // bottom cap ring
    const unsigned int bottomRingStart = static_cast<unsigned int>(vertices.size() / 6);
    for (int i = 0; i < ringSize; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(slices);
        const float angle = t * glm::two_pi<float>();
        const float x = std::cos(angle) * 0.5f;
        const float z = std::sin(angle) * 0.5f;
        vertices.push_back(x); vertices.push_back(-halfHeight); vertices.push_back(z);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
    }

    const unsigned int bottomCenterIndex = static_cast<unsigned int>(vertices.size() / 6);
    vertices.push_back(0.0f); vertices.push_back(-halfHeight); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);

    for (int i = 0; i < slices; ++i) {
        const unsigned int botCurrent = bottomRingStart + i;
        const unsigned int botNext = bottomRingStart + i + 1;
        indices.push_back(bottomCenterIndex);
        indices.push_back(botNext);
        indices.push_back(botCurrent);
    }

    return createMesh(vertices, indices);
}

SceneRenderer::Mesh SceneRenderer::createMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    Mesh mesh;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indices.size());
    return mesh;
}

void SceneRenderer::destroyMesh(Mesh& mesh) {
    if (mesh.VAO) {
        glDeleteVertexArrays(1, &mesh.VAO);
        mesh.VAO = 0;
    }
    if (mesh.VBO) {
        glDeleteBuffers(1, &mesh.VBO);
        mesh.VBO = 0;
    }
    if (mesh.EBO) {
        glDeleteBuffers(1, &mesh.EBO);
        mesh.EBO = 0;
    }
    mesh.indexCount = 0;
}

void SceneRenderer::ensureMesh(PrimitiveType type) {
    if (meshes.find(type) != meshes.end()) {
        return;
    }

    switch (type) {
    case PrimitiveType::Cube:
        meshes[type] = buildCube();
        break;
    case PrimitiveType::Sphere:
        meshes[type] = buildSphere();
        break;
    case PrimitiveType::Cylinder:
        meshes[type] = buildCylinder();
        break;
    case PrimitiveType::Plane:
        meshes[type] = buildPlane();
        break;
    }
}

glm::vec3 SceneRenderer::colorForType(PrimitiveType type) const {
    switch (type) {
    case PrimitiveType::Cube:
        return glm::vec3(0.85f, 0.36f, 0.25f);
    case PrimitiveType::Sphere:
        return glm::vec3(0.25f, 0.65f, 0.95f);
    case PrimitiveType::Cylinder:
        return glm::vec3(0.35f, 0.8f, 0.55f);
    case PrimitiveType::Plane:
        return glm::vec3(0.75f, 0.75f, 0.8f);
    }
    return glm::vec3(0.8f);
}

void SceneRenderer::getDefaultMaterial(glm::vec3& ambient, glm::vec3& diffuse, glm::vec3& specular, float& shininess) const {
    ambient = glm::vec3(0.2f);
    diffuse = glm::vec3(0.8f);
    specular = glm::vec3(0.5f);
    shininess = 32.0f;
}

void SceneRenderer::select(int index) {
    if (index >= 0 && index < static_cast<int>(instances.size())) {
        selectedIndex = index;
    }
}

void SceneRenderer::clearSelection() {
    selectedIndex = -1;
}

void SceneRenderer::translateSelected(const glm::vec3& delta) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(instances.size())) {
        return;
    }
    instances[static_cast<size_t>(selectedIndex)].position += delta;
}

void SceneRenderer::rotateSelected(const glm::vec3& deltaDegrees) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(instances.size())) {
        return;
    }
    instances[static_cast<size_t>(selectedIndex)].rotation += deltaDegrees;
}

void SceneRenderer::scaleSelected(const glm::vec3& deltaScale) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(instances.size())) {
        return;
    }
    auto& inst = instances[static_cast<size_t>(selectedIndex)];
    glm::vec3 adjusted = deltaScale;
    if (inst.type == PrimitiveType::Plane) {
        adjusted.y = 0.0f; // lock height, allow in-plane scaling (x/z)
    }
    inst.scale = glm::max(inst.scale + adjusted, glm::vec3(0.1f));
}

void SceneRenderer::setSelectedPosition(const glm::vec3& position) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(instances.size())) {
        return;
    }
    instances[static_cast<size_t>(selectedIndex)].position = position;
}

PrimitiveInstance* SceneRenderer::getSelectedMutable() {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(instances.size())) {
        return nullptr;
    }
    return &instances[static_cast<size_t>(selectedIndex)];
}

const PrimitiveInstance* SceneRenderer::getSelected() const {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(instances.size())) {
        return nullptr;
    }
    return &instances[static_cast<size_t>(selectedIndex)];
}
