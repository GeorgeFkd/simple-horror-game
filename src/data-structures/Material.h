#pragma once
#include <glm/glm.hpp>
#include <string>

struct Material {
    std::string name;
    glm::vec3 Ka, Kd, Ks; // ambient, diffuse, specular
    float      Ns = 0.0f; // shininess 
    std::string map_Ka, map_Kd, map_Ks; // textures
};