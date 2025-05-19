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
    auto* depth2D  = get_shader_by_name("depth_2d");
    auto* depthCube= get_shader_by_name("depth_cube");

    for (auto* light : lights) {
        if (light->get_type() == LightType::POINT) {
            light->draw_depth_pass(depthCube, models);
        } else {
            light->draw_depth_pass(depth2D,  models);
        }
    }
}

void SceneManager::SceneManager::render(const glm::mat4& view, const glm::mat4& projection){

    // Optional: reset viewport to screen size
    glViewport(0, 0, screen_width, screen_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    Shader* shader = get_shader_by_name("blinn-phong");
    if(!shader){
        throw std::runtime_error("Could not find shader blinn-phong\n");
    }

    shader->use();
    shader->set_int("numLights", (GLint)lights.size());

    for (size_t i = 0; i < lights.size(); ++i) {
        const Light* light = lights[i];
        std::string base = "lights[" + std::to_string(i) + "].";
        light->draw_lighting(shader, base, i);
        light->bind_shadow_map(shader, base, i);
    }

    for (auto const& model: models){
        //model->update_world_transform(glm::mat4(1.0f));
        if(model->is_instanced()){
            model->draw_instanced(view, projection, shader);
        }else{
            model->draw(view, projection, shader);
        }
        //model->draw(lights[0]->get_light_view(), lights[0]->get_light_projection(), shader);
    }


    glUseProgram(0);
}

SceneManager::SceneManager::~SceneManager()
{
    models.clear();
    shaders.clear();
    lights.clear();
}
