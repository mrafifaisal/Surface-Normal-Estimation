#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform2.hpp>

class Camera {
public:
    float fovy = glm::radians(60.0f);
    float aspect = 4.0f/3.0f;

    float alpha = -glm::pi<float>()/2;
    float beta = glm::pi<float>()/2;

    glm::vec2 cliplPlanes = glm::vec2(0.1f, 3000.0f);
    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 at = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4 GetV() const;
    glm::mat4 GetVP() const;
    glm::vec3 GetFWD() const;
    glm::vec3 GetRight() const;

    void MoveTo(glm::vec3 pos);
    void LookAt(glm::vec3 pos);
    void SetFovY(float fovy);
};