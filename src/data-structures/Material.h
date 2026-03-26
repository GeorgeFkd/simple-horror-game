#pragma once
#include <glm/glm.hpp>
#include <string>
struct Material {
    std::string filename;
    std::string name;
    glm::vec3 Ka{0.f};     // ambient
    glm::vec3 Kd{0.f};     // diffuse
    glm::vec3 Ks{0.f};     // specular
    glm::vec3 Ke{0.f};     // emissive
    float     Ns{0.1f};     // shininess
    float     Ni{1.f};     // refraction index (default 1.0)
    float     d {1.f};     // opacity (default 1.0 = opaque)
    int       illum{0};    // illumination model
    std::string map_Ka, map_Kd, map_Ks, map_Bump;
    Material() {};

Material(const glm::vec3& ka, const glm::vec3& kd, const glm::vec3& ks,
             const glm::vec3& ke, float ns, float opacity, float ni, bool bump,
             unsigned int tKa = 0, unsigned int tKd = 0, unsigned int tKs = 0, unsigned int tBump = 0)
        : Ka(ka), Kd(kd), Ks(ks), Ke(ke), Ns(ns), d(opacity), Ni(ni),
          use_bump_map(bump), tex_Ka(tKa), tex_Kd(tKd), tex_Ks(tKs), tex_Bump(tBump) {}

    unsigned int tex_Ka = 0;
    unsigned int tex_Kd = 0;
    unsigned int tex_Ks = 0;
    unsigned int tex_Bump = 0;
    bool   use_bump_map = false;
};
