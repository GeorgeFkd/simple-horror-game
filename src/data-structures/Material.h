#pragma once
#include <glm/glm.hpp>
#include <string>

struct Material {
    std::string name;
    glm::vec3 Ka{0.f};     // ambient
    glm::vec3 Kd{0.f};     // diffuse
    glm::vec3 Ks{0.f};     // specular
    glm::vec3 Ke{0.f};     // emissive
    float     Ns{0.f};     // shininess
    float     Ni{1.f};     // refraction index (default 1.0)
    float     d {1.f};     // opacity (default 1.0 = opaque)
    int       illum{0};    // illumination model
    std::string map_Ka, map_Kd, map_Ks;
};