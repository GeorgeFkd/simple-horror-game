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
        
        inline void debug_dump_model_names() {
            for(auto m : models) {
                std::cout << "Model: " << m->name() << "\n";
            }
        }
        inline void add_model(Models::Model& model){
            models.push_back(&model);
        }
        inline void add_model(Models::Model&& model) {
        models.push_back(std::move(&model));
    }
        inline void add_light(Light& light) { 
            lights.push_back(&light); 
        }

        inline void add_shader(Shader& shader) { 
            shaders.push_back(&shader); 
        }

        inline const std::vector<Models::Model*> get_models() const{
            return models;
        }

        inline std::vector<Light*>& get_lights() {
            return lights;
        }
        
        

        void move_model(Models::Model* model,const glm::vec3& direction) {
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
        
        
        inline void remove_model(Models::Model* model) {
            //we probably need to switch to a hashmap, this costs O(n)   
            auto res = std::remove_if(models.begin(),models.end(),[model](auto* m) { return m->name() == model->name();});
            models.erase(res,models.end());
        }

        inline void remove_instanced_model_at() {throw std::runtime_error("not yet implemented");}

        inline int on_interaction_with(Models::Model* m,std::function<void(SceneManager*)> handler) {
            #if 1 
            std::cout << m->name()  << "-> " << sizeof(handler) << "\n";
            #endif

            eventHandlers[m->name()] = handler;
            return 0;
        }

         inline int on_interaction_with(std::string_view name,std::function<void(SceneManager*)> handler){
            #if 1 
            std::cout << name  << "-> " << sizeof(handler) << "\n";
            #endif    
            eventHandlers.insert({name,handler});
            return 0;
        }

        inline int run_handler_for(Models::Model* m) {
            std::cout << "Run handler for: " << m->name() << "\n";
            std::cout << "Keys: " << eventHandlers.count(m->name()) << "\n";
            assert(eventHandlers.find(m->name()) != eventHandlers.end());
            eventHandlers.at(m->name())(this);
        }

        inline void run_handler_for(std::string_view name) {
            std::cout << "Run handler for: " << name << "\n";
            std::cout << "Keys: " << eventHandlers.count(name) << "\n";
            assert(eventHandlers.find(name) != eventHandlers.end());
            eventHandlers.at(name)(this);
        }
        Shader* get_shader_by_name(const std::string& shader_name);

        void render_depth_pass();
        void render(const glm::mat4& view, const glm::mat4& projection);

        SceneManager(int width, int height):screen_height(height), screen_width(width){};
        ~SceneManager();
        inline Models::Model* findModel(Models::Model* model) {
            auto model_pos = std::find_if(models.begin(),models.end(),[model](auto* m) { return m->name() == model->name();});
            return *model_pos;
        }
        inline Models::Model* findModel(std::string_view name) {
            auto model_pos = std::find_if(models.begin(),models.end(),[name](auto* m) { return m->name() == name;});
            return *model_pos;
        }

        inline Light* findLight(std::string_view name) {
            auto light_pos = std::find_if(lights.begin(),lights.end(),[name](auto* l) { return l->name() == name;});
            return *light_pos;
        }

    private:
         
        std::vector<Models::Model*> models;
        std::vector<Light*> lights;
        std::vector<Shader*> shaders;
        std::unordered_map<std::string_view,std::function<void(SceneManager*)>> eventHandlers = {};
        int screen_width, screen_height;

    };

}

