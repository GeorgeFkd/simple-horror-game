#include "SceneManager.h"
#include <iostream>


Shader* SceneManager::SceneManager::get_shader_by_name(const std::string& shader_name){
    for(auto shader: shaders){
        if(shader->get_shader_name() == shader_name){
            return shader;
        }
    }

    return nullptr;
}

void SceneManager::SceneManager::render_depth_pass() {
    Shader* depth_shader_2d = get_shader_by_name("depth_2d");
    if(!depth_shader_2d){
        throw std::runtime_error("Could not find depth shader 2D\n");
    }
    Shader* depth_shader_cube = get_shader_by_name("depth_cube");
    if(!depth_shader_cube){
        throw std::runtime_error("Could not find depth shader cube\n");
    }
    for (Light* light : lights) {
        if(light->get_type() == LightType::POINT){
            light->draw_depth_pass(depth_shader_cube);
            for (Model::Model* model : models) {
                // ensure transforms are up to date
                model->update_world_transform(glm::mat4(1.0f));
                model->draw_depth(depth_shader_cube);
            }
        }else{
            light->draw_depth_pass(depth_shader_2d);
            for (Model::Model* model : models) {
                // ensure transforms are up to date
                model->update_world_transform(glm::mat4(1.0f));
                model->draw_depth(depth_shader_2d);
            }
        }
        // unbind after each light's pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Optional: reset viewport to screen size
    glViewport(0, 0, screen_width, screen_height);
}

void SceneManager::SceneManager::render(const glm::mat4& view_projection){


    Shader* shader = get_shader_by_name("blinn-phong");
    if(!shader){
        throw std::runtime_error("Could not find shader blinn-phong\n");
    }
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