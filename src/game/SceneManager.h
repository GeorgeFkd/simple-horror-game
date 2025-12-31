#pragma once
#include "Camera.h"
#include "GameState.h"
#include "renderer/Renderer.h"
#include "renderer/TextRenderer.h"
#include "scene_objects/Monster.h"
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
using namespace GlHelpers;
class SceneManager {
  public:
    SceneManager(int width, int height, glm::vec3 camera_position);
    ~SceneManager();
    void debug_dump_model_names();

    // probs should be a move tho
    void set_game_state(Game::GameState& g);
    // inline void set_game_state(Game::GameState& g) {
    //     game_state = &g;
    // }

    inline Game::GameState* get_game_state() {
        return game_state;
    }

    inline const Camera::CameraObj& get_camera() {
        return camera;
    }

    inline void room_dimensions(float width, float height, float depth) {
        room_width  = width;
        room_height = height;
        room_depth  = depth;
    }

    void move_model(const std::string& name, const glm::vec3& direction);
    void move_model_X(const std::string& name, float x);
    void move_model_Y(const std::string& name, float y);
    void move_model_Z(const std::string& name, float z);

    void remove_model(const std::string& name);
    void remove_instanced_model_at(const std::string& name, const std::string& suffix);

    void bind_handler_to_model(const std::string& name, std::function<bool(SceneManager*)> handler);

    void run_game_loop();
    void terminate_game(const std::string& displayed_text);

  private:
    void initialise_opengl_sdl();
    void allocate_game_state_to_gpu();
    void render(const glm::mat4& view, const glm::mat4& projection);
    void handle_sdl_events(bool& running);
    void check_collisions(float dt);
    void perform_culling();
    void run_handler_for(const std::string& m);
    void run_interaction_handlers();
    bool has_user_won();

    Game::GameState*                                                    game_state;
    std::unordered_map<std::string, std::function<bool(SceneManager*)>> event_handlers;
    Renderer                                                            renderer;
    int                                                                 screen_width, screen_height;
    Camera::CameraObj                                                   camera;
    SDL_Window*                                                         window;
    SDL_GLContext                                                       glCtx;
    TextRenderer                                                        text_renderer;
    Monster                                                             monster;
    std::string                                                         center_text       = "";
    std::string                                                         bottom_text_hints = "";
    float                                                               room_width;
    float                                                               room_height;
    float                                                               room_depth;
    glm::vec3                                                           last_camera_position;
    glm::mat4                                                           last_monster_transform;
    bool                                                                running = false;
    unsigned int seconds_to_wait_before_termination                             = 3;
    float        fps                                                            = -1;
    //used in perf measuring
    Uint64 start = 0;
    Uint64 end = 0;
};
}; // namespace Game
