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
#include "TextRenderer.h"
#include <string>
#include "Monster.h"
// REWRITE 1: Use instance suffix-based identification for interaction
// This avoids incorrect handler dispatch after vector shifts due to instance removal
namespace Game {
    using namespace GlHelpers;
    class GameState {
    public:
        GameState()
            : distance_from_closest_model(std::numeric_limits<float>::max()),
              closest_model("") {}

        float distance_from_closest_model;
        std::string closest_model;

        /// Take ownership of this model and register it under `name`.
        void add_model(std::unique_ptr<Models::Model> model, const std::string& name);
        void add_model(Models::Model&& model, const std::string& name);

        /// Remove (and destroy) the model registered as `name` (if any).
        void remove_model(const std::string& name);

        /// Look up a model by name; returns nullopt if not found.
        Models::Model* find_model(const std::string& name) const;

        /// Fast, cache-friendly iteration over all models.
        const std::vector<std::unique_ptr<Models::Model>>& get_models() const;

        // — Lights API —
        /// Take ownership of this light and register it under `name`.
        void add_light(std::unique_ptr<Light> light, const std::string& name);
        void add_light(Light&& light, const std::string& name);

        /// Remove (and destroy) the light registered as `name` (if any).
        void remove_light(const std::string& name);

        /// Look up a light by name; returns nullopt if not found.
        Light* find_light(const std::string& name) const;

        /// Fast, cache-friendly iteration over all lights.
        const std::vector<std::unique_ptr<Light>>& get_lights() const;

        unsigned int pages_collected = 0;
        unsigned int pages_collected_to_win = 6;
        void clear_models() {
            models.clear();          // the unique_ptrs—and hence the Model objects—are destroyed
            model_names.clear();     // clear the name list
            model_indices.clear();   // clear the lookup map
        }

        void clear_lights() {
            lights.clear();
            light_names.clear();
            light_indices.clear();
        }
    private:
        std::vector<std::unique_ptr<Models::Model>> models;
        std::vector<std::string>                   model_names;
        std::unordered_map<std::string, size_t>    model_indices;

        std::vector<std::unique_ptr<Light>> lights;
        std::vector<std::string>           light_names;
        std::unordered_map<std::string, size_t> light_indices;
    };

    class SceneManager {
    public:
        SceneManager(int width, int height, Camera::CameraObj camera);
        ~SceneManager();
        void debug_dump_model_names();

        inline void set_game_state(GameState& g) {
            game_state = &g;
        }

        inline GameState* get_game_state() {
            return game_state;
        }
        inline void add_shader(std::shared_ptr<Shader> shader) {
            shaders.push_back(shader);
        }

        inline const Camera::CameraObj& get_camera() {
            return camera;
        }

        void move_model(const std::string& name, const glm::vec3& direction);
        void move_model_X(const std::string& name, float x);
        void move_model_Y(const std::string& name, float y);
        void move_model_Z(const std::string& name, float z);

        void remove_model(const std::string& name);
        void remove_instanced_model_at(const std::string& name, const std::string& suffix);

        void bind_handler_to_model(const std::string& name, std::function<bool(SceneManager*)> handler);

        void initialise_shaders();
        void initialise_opengl_sdl();
        void run_game_loop();
        void terminate_game(const std::string& displayed_text);
    private:
        void render_depth_pass();
        void render(const glm::mat4& view, const glm::mat4& projection);
        std::shared_ptr<Shader> get_shader_by_name(const std::string& shader_name);
        void handle_sdl_events(bool& running);
        void check_collisions(float dt);
        void perform_culling();
        void run_handler_for(const std::string& m);
        void run_interaction_handlers();
        bool has_user_won();

        GameState* game_state;
        std::vector<std::shared_ptr<Shader>> shaders;
        std::unordered_map<std::string, std::function<bool(SceneManager*)>> event_handlers;
        int screen_width, screen_height;
        Camera::CameraObj camera;
        SDL_Window* window;
        SDL_GLContext glCtx;
        TextRenderer text_renderer;
        Monster monster;
        std::string center_text = "";
        std::string bottom_text_hints = "";
        glm::vec3 last_camera_position;
        glm::mat4 last_monster_transform;
        bool running = false;
        unsigned int seconds_to_wait_before_termination = 5;
    };
};
