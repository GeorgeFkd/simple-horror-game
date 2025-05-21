#pragma once
#include "Camera.h"
#include "Light.h"
#include "Model.h"
#include "Shader.h"
#include <GL/glew.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <string>

namespace Game {

class GameState {
    
    // GameState: <Models,Lights,Closest Entity,Camera>
  public:

    GameState(){};
    std::vector<Models::Model*> models;
    std::vector<Light*>         lights;
    float                       distanceFromClosestModel;
    std::string_view            closestModelName;
    // const Camera::CameraObj&           camera;

    Models::Model* findModel(Models::Model* model);
    Models::Model* findModel(std::string_view name);
    Light*         findLight(std::string_view name);
    inline void add_model(Models::Model& model) {
        models.push_back(&model);
    }
    inline void add_model(Models::Model&& model) {
        models.push_back(std::move(&model));
    }
    inline void add_light(Light& light) {
        lights.push_back(&light);
    }

    
    // inline void set_camera(Camera::CameraObj camera) {
    //     this->camera = camera;
    // }
    //
    inline const std::vector<Models::Model*> get_models() const {
        return models;
    }

    inline std::vector<Light*>& get_lights() {
        return lights;
    }
};

//SceneManager->Game.run(InitialState<GameState>)
//PreparationSteps: [InitialiseShaders,Setup Models(Instanced and normal ones),Setup Lights,Setup Interaction handlers]
//GameLoop: [Keyboard Handlers(Events from SDL),UpdateCamera vectors,Iterate over models(For collisions interactions etc. etc.),Render]
//In the destructor unbind all the stuff needed from opengl and free memory
class SceneManager {
  public:
    SceneManager(int width, int height);
    ~SceneManager();
    void        debug_dump_model_names();
    inline void set_game_state(GameState g) { gameState = g;}
    inline void add_shader(Shader& shader) {
        shaders.push_back(&shader);
    }

    inline const std::vector<Models::Model*> get_models() const {
        return gameState.models;
    }

    inline std::vector<Light*>& get_lights() {
        return gameState.lights;
    }

    // i should also write the rotate methods
    void move_model(Models::Model* model, const glm::vec3& direction);
    void move_model(std::string_view name, const glm::vec3& direction);
    void move_model_X(std::string_view name, float x);
    void move_model_Y(std::string_view name, float y);
    void move_model_Z(std::string_view name, float z);

    void    remove_model(Models::Model* model);
    void    remove_instanced_model_at();
    int     on_interaction_with(Models::Model* m, std::function<void(SceneManager*)> handler);
    int     on_interaction_with(std::string_view name, std::function<void(SceneManager*)> handler);
    int     run_handler_for(Models::Model* m);
    void    run_handler_for(std::string_view name);
    Shader* get_shader_by_name(const std::string& shader_name);

    void           render_depth_pass();
    void           render(const glm::mat4& view, const glm::mat4& projection);
    Models::Model* findModel(Models::Model* model);
    Models::Model* findModel(std::string_view name);
    Light*         findLight(std::string_view name);
    void run(GameState initialState);
  private:
    
    void initialiseOpenGL_SDL();
    void initialiseShaders();
    void setupGameState();
//PreparationSteps: [InitialiseShaders,Setup Models(Instanced and normal ones),Setup Lights,Setup Interaction handlers]
    GameState gameState;
    std::vector<Shader*>                                                     shaders;
    std::unordered_map<std::string_view, std::function<void(SceneManager*)>> eventHandlers = {};
    int screen_width, screen_height;

};

}; // namespace Game
