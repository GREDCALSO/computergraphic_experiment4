#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    enum class MoveDir { Forward, Backward, Left, Right };

    Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 5.0f),
        const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = 0.0f);

    glm::mat4 GetViewMatrix() const;

    void ProcessKeyboard(MoveDir direction, float deltaTime);
    void Dolly(float offset);
    void AdjustSpeed(float offset);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    float GetSpeed() const { return MovementSpeed; }
    glm::vec3 GetPosition() const { return Position; }
    glm::vec3 GetFront() const { return Front; }

private:
    void updateVectors();

    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
};
