#include "camera.h"

#include <algorithm>

Camera::Camera(const glm::vec3& position, const glm::vec3& up, float yaw, float pitch)
    : Position(position), WorldUp(up), Yaw(yaw), Pitch(pitch), MovementSpeed(3.0f), MouseSensitivity(0.1f) {
    updateVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(MoveDir direction, float deltaTime) {
    const float velocity = MovementSpeed * deltaTime;
    if (direction == MoveDir::Forward) {
        Position += Front * velocity;
    }
    else if (direction == MoveDir::Backward) {
        Position -= Front * velocity;
    }
    else if (direction == MoveDir::Left) {
        Position -= Right * velocity;
    }
    else if (direction == MoveDir::Right) {
        Position += Right * velocity;
    }
}

void Camera::Dolly(float offset) {
    const float dollyScale = 1.0f;
    Position += Front * static_cast<float>(offset) * dollyScale;
}

void Camera::AdjustSpeed(float offset) {
    const float factor = 1.0f + static_cast<float>(offset) * 0.1f;
    MovementSpeed = std::max(0.1f, MovementSpeed * factor);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        Pitch = std::clamp(Pitch, -89.0f, 89.0f);
    }

    updateVectors();
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
