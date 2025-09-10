#include "camera.hpp"

glm::mat4 Camera::GetV() const {
    return glm::lookAt(pos, at, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetVP() const {
    return glm::perspective(fovy, aspect, cliplPlanes.x, cliplPlanes.y) *
           GetV();
}

glm::vec3 Camera::GetFWD() const { return glm::normalize(at - pos); }

glm::vec3 Camera::GetRight() const {
    return glm::normalize(glm::cross(at - pos, glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Camera::MoveTo(glm::vec3 p) {
    at = p + at - pos;
    pos = p;
}

void Camera::LookAt(glm::vec3 p) {
    glm::vec3 dir = glm::normalize(p - pos);
    at = dir + pos;
    alpha = atan2(dir.z, dir.x);
    beta = atan2(sqrt(dir.x*dir.x+dir.z*dir.z), dir.y);
}

void Camera::SetFovY(float fovy) {
    this->fovy = fovy;
}