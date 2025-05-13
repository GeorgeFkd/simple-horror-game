#include "SceneManager.h"
#include <iostream>


Shader* SceneManager::SceneManager::get_shader_by_name(const std::string& shader_name){
    for(auto shader: shaders){
        if(shader->get_shader_name() == shader_name){
            return shader;
        }
    }
}

void SceneManager::SceneManager::render(const glm::mat4& view_projection){


    Shader* shader = get_shader_by_name("blinn-phong");
    GLuint shader_program_id = shader->get_shader_program_id();
    glUseProgram(shader_program_id);

    GLint locNum = glGetUniformLocation(shader_program_id, "numLights");
    glUniform1i(locNum, (GLint)lights.size());

    for (size_t i = 0; i < lights.size(); ++i) {
        Light* L = lights[i];
        std::string base = "lights[" + std::to_string(i) + "].";

        glUniform3fv(
            glGetUniformLocation(shader_program_id, (base + "position").c_str()),
            1, glm::value_ptr(L->position)
        );
        glUniform3fv(
            glGetUniformLocation(shader_program_id, (base + "direction").c_str()),
            1, glm::value_ptr(L->direction)
        );
        glUniform3fv(
            glGetUniformLocation(shader_program_id, (base + "ambient").c_str()),
            1, glm::value_ptr(L->ambient)
        );
        glUniform3fv(
            glGetUniformLocation(shader_program_id, (base + "diffuse").c_str()),
            1, glm::value_ptr(L->diffuse)
        );
        glUniform3fv(
            glGetUniformLocation(shader_program_id, (base + "specular").c_str()),
            1, glm::value_ptr(L->specular)
        );
        glUniform1f(
            glGetUniformLocation(shader_program_id, (base + "cutoff").c_str()),
            L->cutoff
        );
        glUniform1f(
            glGetUniformLocation(shader_program_id, (base + "outerCutoff").c_str()),
            L->outerCutoff
        );
        glUniform1i(
            glGetUniformLocation(shader_program_id, (base + "type").c_str()),
            (int)L->type
        );
    }
    for (auto const& model: models){
        model->update_world_transform(glm::mat4(1.0f));
        model->draw(view_projection);
    }

    glUseProgram(0);
}

SceneManager::SceneManager::~SceneManager()
{
    models.clear();
    shaders.clear();
    lights.clear();
}