#pragma once
#include <glm/glm.hpp>

enum LightType { POINT=0, DIRECTIONAL=1, SPOT=2 };

struct Light {
    LightType type        = POINT;
    glm::vec3 position    = glm::vec3(0.0f);
    glm::vec3 direction   = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 ambient     = glm::vec3(0.1f);
    glm::vec3 diffuse     = glm::vec3(0.8f);
    glm::vec3 specular    = glm::vec3(1.0f);
    float     cutoff      = glm::cos(glm::radians(12.5f));  // inner cone
    float     outerCutoff = glm::cos(glm::radians(17.5f));  // outer cone
};