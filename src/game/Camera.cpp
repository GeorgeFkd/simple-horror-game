#include "Camera.h"
#include <array>
void Camera::CameraObj::update(float delta_time,bool forward,bool backward,bool leftMoved,bool rightMoved,bool up,bool down) {
    float velocity = camera_speed * delta_time;

    glm::vec3 flat_front = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 flat_right = glm::normalize(glm::vec3(right.x, 0.0f, right.z));

    if (forward)
        position += flat_front * velocity;
    if (backward)
        position -= flat_front * velocity;
    if (leftMoved)
        position -= flat_right * velocity;
    if (rightMoved)
        position += flat_right * velocity;
    if (up)
        position += world_up * velocity;
    if (down)
        position -= world_up * velocity;
}

void Camera::CameraObj::updateCameraVectors() {
    // calculate new front vector
    glm::vec3 f;
    // contribution on x axis * how much we are rotated on the y
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    // contribution on y axis
    f.y = sin(glm::radians(pitch));
    // contribution on z axis * how much we are rotated on the y
    f.z   = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(f);

    // re‐compute right and up
    right = glm::normalize(glm::cross(front, world_up));
    up    = glm::normalize(glm::cross(right, front));
}

void Camera::CameraObj::resizeWindow(float width,float height){
    assert(height != 0);
    aspect = width / height;
    updateCameraVectors();
}

void Camera::CameraObj::move(int xrel, int yrel) {
    yaw += xrel * mouse_sensitivity;
    pitch -= yrel * mouse_sensitivity;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    updateCameraVectors();
}

const std::array<glm::vec4, 6> Camera::CameraObj::extract_frustum_planes() const {

    const glm::mat4          M = get_projection_matrix() * get_view_matrix();
    std::array<glm::vec4, 6> P;

    glm::mat4 T = glm::transpose(M);
    P[0]        = T[3] + T[0]; // left
    P[1]        = T[3] - T[0]; // right
    P[2]        = T[3] + T[1]; // bottom
    P[3]        = T[3] - T[1]; // top
    P[4]        = T[3] + T[2]; // near
    P[5]        = T[3] - T[2]; // far

    // Normalize
    for (auto& p : P) {
        float len = glm::length(glm::vec3(p));
        p /= len;
    }

    return P;
}
