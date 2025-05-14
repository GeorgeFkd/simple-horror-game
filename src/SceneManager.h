#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <string>
#include <algorithm>
#include "Model.h"
#include "Light.h"
#include "Shader.h"

namespace SceneManager{


    class SceneManager{
    public: 

        inline void add_model(Model::Model& model){
            models.push_back(&model);
        }

        inline void add_light(Light& light) { 
            lights.push_back(&light); 
        }

        inline void add_shader(Shader& shader) { 
            shaders.push_back(&shader); 
        }

        inline const std::vector<Model::Model*> get_models() const{
            return models;
        }

        inline void set_spotlight(size_t idx, glm::vec3& pos, glm::vec3& dir) {
            if (idx < lights.size()) {
                lights[idx]->set_position(pos);
                lights[idx]->set_direction(dir);
            }
        }

        Shader* get_shader_by_name(const std::string& shader_name);

        void render_depth_pass();
        void render(const glm::mat4& view_projection);

        SceneManager(int width, int height):screen_height(height), screen_width(width){};
        ~SceneManager();
    private:

        std::vector<Model::Model*> models;
        std::vector<Light*> lights;
        std::vector<Shader*> shaders;

        int screen_width, screen_height;

    };

}