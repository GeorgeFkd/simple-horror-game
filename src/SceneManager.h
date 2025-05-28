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
// REWRITE 1: Use instance suffix-based identification for interaction
// This avoids incorrect handler dispatch after vector shifts due to instance removal
namespace Game {
    class GameState {
    public:
        GameState()
            : distanceFromClosestModel(std::numeric_limits<float>::max()),
              closestModel("") {}

        float distanceFromClosestModel;
        std::string closestModel;

        Models::Model* findModel(Models::Model* model);
        Models::Model* findModel(std::string_view name);
        Light* findLight(std::string_view name);

        void add_model(Models::Model& model) {
            models.push_back(&model);
        }

        void add_light(Light& light) {
            lights.push_back(&light);
        }

        std::vector<Models::Model*> get_models() const {
            return models;
        }

        std::vector<Light*>& get_lights() {
            return lights;
        }

        void remove_model(Models::Model* model);

    private:
        std::vector<Models::Model*> models;
        std::vector<Light*> lights;
    };

    class SceneManager {
    public:
        SceneManager(int width, int height, Camera::CameraObj camera);
        ~SceneManager();
        void debug_dump_model_names();
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
        void remove_instanced_model_at(Models::Model* m, const std::string& suffix);

        int on_interaction_with(std::string_view name, std::function<void(SceneManager*)> handler);

        void initialise_shaders();
        void run_game_loop();
        void initialise_opengl_sdl();

    private:
        void run_handler_for(const std::string& m);
        void render_depth_pass();
        void render(const glm::mat4& view, const glm::mat4& projection);
        std::shared_ptr<Shader> get_shader_by_name(const std::string& shader_name);
        void handle_sdl_events(bool& running);
        void check_all_models(float dt);
        void run_interaction_handlers();

        GameState gameState;
        Models::Model* monster;
        std::vector<std::shared_ptr<Shader>> shaders;
        std::unordered_map<std::string, std::function<void(SceneManager*)>> eventHandlers;
        int screen_width, screen_height;
        Camera::CameraObj camera;
        glm::vec3 last_camera_position;
        glm::mat4 last_monster_transform;
        SDL_Window* window;
        SDL_GLContext glCtx;
    };
};
