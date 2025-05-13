#include "Light.h"


void Light::draw_lighting(GLuint shader_program_id, const std::string& base){

    glUniform3fv(
        glGetUniformLocation(shader_program_id, (base + "position").c_str()),
        1, glm::value_ptr(position)
    );
    
    glUniform3fv(
        glGetUniformLocation(shader_program_id, (base + "direction").c_str()),
        1, glm::value_ptr(direction)
    );
    glUniform3fv(
        glGetUniformLocation(shader_program_id, (base + "ambient").c_str()),
        1, glm::value_ptr(ambient)
    );
    glUniform3fv(
        glGetUniformLocation(shader_program_id, (base + "diffuse").c_str()),
        1, glm::value_ptr(diffuse)
    );
    glUniform3fv(
        glGetUniformLocation(shader_program_id, (base + "specular").c_str()),
        1, glm::value_ptr(specular)
    );
    glUniform1f(
        glGetUniformLocation(shader_program_id, (base + "cutoff").c_str()),
        cutoff
    );
    glUniform1f(
        glGetUniformLocation(shader_program_id, (base + "outerCutoff").c_str()),
        outer_cutoff
    );
    glUniform1i(
        glGetUniformLocation(shader_program_id, (base + "type").c_str()),
        (int)type
    );

}