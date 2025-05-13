#include "Light.h"


void Light::draw_lighting(Shader* shader, const std::string& base) const{
    // use the cached location API instead of raw glGetUniformLocation
    glUniform3fv(shader->get_uniform_location(base + "position"),  1, glm::value_ptr(position));
    // only send direction for non-point lights
    if (type != LightType::POINT)
        glUniform3fv(shader->get_uniform_location(base + "direction"), 1, glm::value_ptr(direction));
        glUniform3fv(shader->get_uniform_location(base + "ambient"),   1, glm::value_ptr(ambient));
        glUniform3fv(shader->get_uniform_location(base + "diffuse"),   1, glm::value_ptr(diffuse));
        glUniform3fv(shader->get_uniform_location(base + "specular"),  1, glm::value_ptr(specular));
    if (type == LightType::SPOT) {
        glUniform1f(shader->get_uniform_location(base + "cutoff"),      cutoff);
        glUniform1f(shader->get_uniform_location(base + "outerCutoff"), outer_cutoff);
    }

    glUniform1i(shader->get_uniform_location(base + "type"), (int)type);
}