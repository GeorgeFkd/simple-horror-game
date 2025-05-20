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
        inline void add_model(Model::Model&& model) {
        models.push_back(std::move(&model));
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
        
        

        void move_model(Model::Model* model,const glm::vec3& direction) {
            auto model_pos = findModel(model);
            model_pos->move_relative_to(direction);
        }

        void move_model(std::string_view name,const glm::vec3& direction) {
            auto model_pos = findModel(name);
            model_pos->move_relative_to(direction);
        }

        void move_model_X(std::string_view name,float x) {
            move_model(name,glm::vec3(x,0.0f,0.0f));
        }
        
        void move_model_Y(std::string_view name,float y) {
            move_model(name,glm::vec3(0.0f,y,0.0f));
        }

        void move_model_Z(std::string_view name, float z) {
            move_model(name,glm::vec3(0.0f,0.0f,z));
        }
        
        
        inline void remove_model(Model::Model* model) {
            //we probably need to switch to a hashmap, this costs O(n)   
            auto res = std::remove_if(models.begin(),models.end(),[model](auto* m) { return m->name() == model->name();});
            models.erase(res,models.end());
        }

        inline void remove_instanced_model_at() {throw std::runtime_error("not yet implemented");}

        

        Shader* get_shader_by_name(const std::string& shader_name);

        void render_depth_pass();
        void render(const glm::mat4& view, const glm::mat4& projection);

        SceneManager(int width, int height):screen_height(height), screen_width(width){};
        ~SceneManager();
    private:
        inline Model::Model* findModel(Model::Model* model) {
            auto model_pos = std::find_if(models.begin(),models.end(),[model](auto* m) { return m->name() == model->name();});
            return *model_pos;
        }
        inline Model::Model* findModel(std::string_view name) {
            auto model_pos = std::find_if(models.begin(),models.end(),[name](auto* m) { return m->name() == name;});
            return *model_pos;
        } 
        std::vector<Model::Model*> models;
        std::vector<Light*> lights;
        std::vector<Shader*> shaders;

        int screen_width, screen_height;

    };

}

