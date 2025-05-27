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
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace Game {
// Todo: Textures,Plot/Game Design/Scenarios
// Also I want some things to be easily configurable: Creating a room of
// size width*height*length Make doors(it is just a model + translation + rotation)

typedef std::tuple<std::string_view, std::optional<size_t>> ModelInstance;
struct ModelInstanceHasher {
    size_t operator()(const ModelInstance& m) const {
        size_t      seed = 0;
        const auto& str  = std::get<0>(m);
        const auto& opt  = std::get<1>(m);

        // Combine hash of string_view
        seed ^= std::hash<std::string_view>{}(str) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        // Combine hash of optional<size_t>
        if (opt.has_value()) {
            seed ^= std::hash<size_t>{}(*opt) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed;
    }
};

class GameState {
  public:
    GameState() {
        distanceFromClosestModel = std::numeric_limits<float>::max();
        closestModel             = std::make_tuple("", std::nullopt);
    };

    float         distanceFromClosestModel;
    ModelInstance closestModel;

    Models::Model* findModel(Models::Model* model);
    Models::Model* findModel(std::string_view name);
    Light*         findLight(std::string_view name);
    inline void    add_model(Models::Model& model) {
        models.push_back(&model);
    }

    inline void add_light(Light& light) {
        lights.push_back(&light);
    }

    inline std::vector<Models::Model*> get_models() const {
        return models;
    }

    inline std::vector<Light*>& get_lights() {
        return lights;
    }
    void remove_model(Models::Model* model);

  private:
    std::vector<Models::Model*> models;
    std::vector<Light*>         lights;
};

class SceneManager {
  public:
    SceneManager(int width, int height, Camera::CameraObj camera);
    ~SceneManager();
    void        debug_dump_model_names();
    inline void set_game_state(GameState g) {
        gameState = g;
    }

    inline GameState* get_game_state() {
        return &gameState;
    }
    inline void add_shader(std::shared_ptr<Shader> shader) {
        shaders.push_back(shader);
    }

    inline const Camera::CameraObj& get_camera() {
        return camera;
    }

    void move_model(Models::Model* model, const glm::vec3& direction);
    void move_model(std::string_view name, const glm::vec3& direction);
    void move_model_X(std::string_view name, float x);
    void move_model_Y(std::string_view name, float y);
    void move_model_Z(std::string_view name, float z);

    void remove_model(Models::Model* m);
    void remove_instanced_model_at(Models::Model* m, size_t instancePos);

    int on_interaction_with(std::string_view name, std::function<void(SceneManager*)> handler);
    int on_interaction_with_instance(std::string_view name, size_t instancePos,
                                     std::function<void(SceneManager*)> handler);

    void initialiseShaders();
    void runGameLoop();
    void initialiseOpenGL_SDL();

  private:
    void                    run_handler_for(const ModelInstance& m);
    void                    render_depth_pass();
    void                    render(const glm::mat4& view, const glm::mat4& projection);
    std::shared_ptr<Shader> get_shader_by_name(const std::string& shader_name);
    void                    handleSDLEvents(bool& running);
    void                    checkAllModels(float dt);
    void                    runInteractionHandlers();

    GameState                            gameState;
    std::vector<std::shared_ptr<Shader>> shaders;
    std::unordered_map<ModelInstance, std::function<void(SceneManager*)>, ModelInstanceHasher>
                      eventHandlers;
    int               screen_width, screen_height;
    Camera::CameraObj camera;
    glm::vec3         last_camera_position;
    SDL_Window*       window;
    SDL_GLContext     glCtx;
};

}; // namespace Game
