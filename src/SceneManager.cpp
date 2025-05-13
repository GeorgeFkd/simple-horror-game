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
    shader->use();

    GLint locNum = shader->get_uniform_location("numLights");
    glUniform1i(locNum, (GLint)lights.size());

    for (size_t i = 0; i < lights.size(); ++i) {
        const Light* light = lights[i];
        std::string base = "lights[" + std::to_string(i) + "].";
        light->draw_lighting(shader, base);
    }

    for (auto const& model: models){
        model->update_world_transform(glm::mat4(1.0f));
        model->draw(view_projection, shader);
    }

    glUseProgram(0);
}

SceneManager::SceneManager::~SceneManager()
{
    models.clear();
    shaders.clear();
    lights.clear();
}