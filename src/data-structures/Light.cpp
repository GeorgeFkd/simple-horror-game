#include "Light.h"
#include "EntityID.h"
#include "Shader.h"
#include <array>
#include <iostream>
#include <memory>
Light::Light(LightType light_type, const glm::vec3& position, const glm::vec3& direction,
             const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
             float cutoff,       // inner cone
             float outer_cutoff, // outer cone
             int shadow_width, int shadow_height, float near_plane, float far_plane,
             float ortho_size, float attenuation_constant, float attenuation_linear,
             float attenuation_quadratic, float attenuation_power, float light_power, bool is_on,
             std::string_view label, glm::vec3 color)
    : type(light_type), position(position), direction(direction), ambient(ambient),
      diffuse(diffuse), specular(specular), cutoff(cutoff), outer_cutoff(outer_cutoff),
      shadow_height(shadow_height), shadow_width(shadow_width), near_plane(near_plane),
      far_plane(far_plane), ortho_size(ortho_size), attenuation_constant(attenuation_constant),
      attenuation_linear(attenuation_linear), attenuation_quadratic(attenuation_quadratic),
      attenuation_power(attenuation_power), light_power(light_power), is_on(is_on), label(label),
      color(color) {
    
    id = next_entity_id();
}

std::array<glm::vec4, 6> Light::extract_frustum_planes(const glm::mat4& M) {
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




glm::mat4 Light::get_light_projection() const {

    switch (type) {
    case LightType::DIRECTIONAL: {
        // Ortho dims: half‐width/height of the shadow volume
        float orthoSize = this->ortho_size;
        return glm::ortho(-ortho_size, +ortho_size, -ortho_size, +ortho_size, near_plane,
                          far_plane);
    }
    case LightType::POINT: {
        // For point lights we render a cubemap: 90° FOV and square aspect
        float aspect = (float)shadow_width / (float)shadow_height;
        return glm::perspective(glm::radians(90.0f), aspect, near_plane, far_plane);
    }
    case LightType::SPOT: {
        // Use the spot cone angle as FOV (double the cutoff half‐angle)
        float fov    = glm::radians(outer_cutoff * 50.0f);
        float aspect = (float)shadow_width / (float)shadow_height;
        return glm::perspective(
            // fov,
            glm::radians(90.0f), aspect, near_plane, far_plane);
    }
    }
    assert(false);
}

glm::mat4 Light::get_light_view() const {
    // For directional and spot lights
    auto      dir = glm::normalize(direction);
    glm::vec3 up = (abs(dir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::lookAt(position, position + dir, up);
}

std::vector<glm::mat4> Light::get_point_light_views() const {
    return {
        glm::lookAt(position, position + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),  // +X
        glm::lookAt(position, position + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)), // -X
        glm::lookAt(position, position + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),   // +Y
        glm::lookAt(position, position + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)), // -Y
        glm::lookAt(position, position + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),  // +Z
        glm::lookAt(position, position + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))  // -Z
    };
}
