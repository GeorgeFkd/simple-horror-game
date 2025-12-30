#include "SceneManager.h"
#include "Camera.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL_keyboard.h>
#include <SDL_timer.h>
#include <algorithm>
#include <chrono>
#include <glm/gtx/vector_angle.hpp>
#include <iostream>
#include <random>

void Game::SceneManager::initialise_opengl_sdl() {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {
        std::cerr << "Something went wrong when initialising SDL\n";
        return;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 4, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError()
                  << std::endl;
        return;
    }

    

    // SDL_Window*
    window = SDL_CreateWindow("Old room", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    // SDL_GLContext
    glCtx = SDL_GL_CreateContext(window);


    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    renderer.init(screen_width,screen_height);
    renderer.initialise_shaders();
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

#define PRINT_VEC4(v)                                                                              \
    std::cout << #v " = (" << (v).x << ", " << (v).y << ", " << (v).z << ", " << (v).w << ")"      \
              << "(in main)" << std::endl

void Game::SceneManager::terminate_game(const std::string& text) {
    center_text = text;
    running     = false;
}

bool Game::SceneManager::has_user_won() {
    return game_state->pages_collected >= game_state->pages_collected_to_win;
}

void Game::SceneManager::run_game_loop() {

    allocate_game_state_to_gpu();

    Mix_Music* horror_music = Mix_LoadMUS("assets/audio/scary.mp3");
    if (!horror_music) {
        std::cerr << "Failed to load background music: " << Mix_GetError() << "\n";
        return;
    }

    Mix_Chunk* footsteps_sound         = Mix_LoadWAV("assets/audio/footsteps.mp3");
    int        footsteps_sound_channel = 2;
    if (!footsteps_sound) {
        std::cerr << "Failed to load music for footsteps: " << Mix_GetError() << "\n";
        return;
    }

    auto monster_initial_position = glm::vec3(5.0f, 0.0f, 5.0f);
    last_monster_transform        = glm::translate(glm::mat4(1.0f), monster_initial_position);
    auto monster_init             = Models::Model("assets/models/monster.obj", "monster");
    game_state->add_model(std::move(monster_init), "monster");
    allocate_game_state_to_gpu();
    auto monster_model = game_state->find_model("monster");
    if (!monster_model) {
        throw std::runtime_error("Could not find monster model before starting game...");
    }
    monster_model->set_local_transform(last_monster_transform);
    monster_model->update_world_transform(glm::mat4(1.0f));
    Monster monster(monster_model);
    monster.disappear_probability(0.65f)
        .seconds_for_coinflip(15.0f)
        .speed_within(4.0f, 10.0f)
        .restrict_monster_within(-room_width, room_width, -room_depth, room_depth);

    running           = true;
    Uint64 lastTicks  = SDL_GetPerformanceCounter();
    auto   flashlight = game_state->find_light("flashlight");

    if (!flashlight) {
        throw std::runtime_error("Could not find flashlight model...");
    }

    monster.on_monster_active([&](auto m) {
        if (Mix_PlayingMusic() == 0) {
            Mix_PlayMusic(horror_music, -1); // -1 = loop forever
            Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
            Mix_PlayChannel(footsteps_sound_channel, footsteps_sound, -1);
        }
    });

    monster.on_monster_not_active([footsteps_sound_channel](auto m) {
        if (Mix_PlayingMusic()) {
            Mix_HaltMusic();
            Mix_HaltChannel(footsteps_sound_channel);
        }
    });
    monster.on_chase_start([]() {
        if (Mix_PlayingMusic()) {
            Mix_VolumeMusic(MIX_MAX_VOLUME);
        }
    });
    monster.on_chase_stop([]() {
        if (Mix_PlayingMusic()) {
            Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
        }
    });
    monster.set_chasing_speed(4.0f);

    while (running) {
        if (has_user_won()) {
            // runs one more iteration so it can display the text
            terminate_game("You Won!");
        }
        Uint64 now = SDL_GetPerformanceCounter();
        float  dt  = float(now - lastTicks) / float(SDL_GetPerformanceFrequency());
        assert(dt != 0);
        fps = 1/dt;
        lastTicks  = now;
        handle_sdl_events(running);
        last_camera_position   = camera.get_position();
        last_monster_transform = monster.monster_model()->get_local_transform();
        camera.update(dt);
        // std::cout << "Last camera position and current: \n";
        auto camera_dir = glm::normalize(camera.get_direction());
        monster.update(dt, camera_dir, last_camera_position);
        if (monster.monster_model()->is_active()) {
            glm::vec3 cam_pos    = camera.get_position();
            glm::mat4 mon_tf     = monster.monster_model()->get_local_transform();
            glm::vec3 mon_pos    = glm::vec3(mon_tf[3]);
            glm::vec3 to_monster = mon_pos - cam_pos;
            glm::vec3 dir_xz     = glm::normalize(glm::vec3(to_monster.x, 0.0f, to_monster.z));
            glm::vec3 forward    = camera.get_direction();
            glm::vec3 forward_xz = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));

            float angle_rad = glm::orientedAngle(forward_xz, dir_xz, glm::vec3(0.0f, 1.0f, 0.0f));
            float angle_deg = glm::degrees(angle_rad);

            float distance_f = glm::length(to_monster);
            distance_f       = std::clamp(distance_f, 0.0f, 255.0f);

            uint8_t distance_byte = static_cast<uint8_t>(distance_f + 0.5f);
            //mb i mean distance byte here? 
            Mix_SetPosition(footsteps_sound_channel, angle_deg, distance_f);
        }

        check_collisions(dt);

        float     right_offset = 0.4f;
        glm::vec3 offset =
            right_offset * camera.get_right(); // + forward_offset * camera.get_direction()
        flashlight->set_position(camera.get_position() + offset);
        flashlight->set_direction(camera.get_direction());

        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 proj = camera.get_projection_matrix();
        //TODO: add this when properly fixed
        //  perform_culling();
        render(view, proj);
        run_interaction_handlers();
        SDL_GL_SwapWindow(window);
    }
    Mix_FreeChunk(footsteps_sound);
    Mix_FreeMusic(horror_music);
    SDL_Delay(seconds_to_wait_before_termination * 1000);
}



void Game::SceneManager::move_model(const std::string& name, const glm::vec3& direction) {
    auto model_pos = game_state->find_model(name);
    if (!model_pos) {
        throw std::runtime_error("Cannot find model: " + name);
    }
    model_pos->move_relative_to(direction);
}

void Game::SceneManager::move_model_X(const std::string& name, float x) {
    move_model(name, glm::vec3(x, 0.0f, 0.0f));
}

void Game::SceneManager::move_model_Y(const std::string& name, float y) {
    move_model(name, glm::vec3(0.0f, y, 0.0f));
}

void Game::SceneManager::move_model_Z(const std::string& name, float z) {
    move_model(name, glm::vec3(0.0f, 0.0f, z));
}

void Game::SceneManager::remove_model(const std::string& name) {
    auto it = event_handlers.find(name);
    if (it != event_handlers.end()) {
        std::cout << "Removing model: " << name << "\n";
        game_state->remove_model(name);
    }
}

void Game::SceneManager::bind_handler_to_model(const std::string&                 name,
                                               std::function<bool(SceneManager*)> handler) {

    event_handlers.insert({name, handler});
}

void Game::SceneManager::run_handler_for(const std::string& m) {
    auto it = event_handlers.find(m);
    if (it != event_handlers.end()) {
        std::cout << "Running event handler for: " << m << "\n";
        bool keep = it->second(this);
        if (!keep) {
            it = event_handlers.erase(it);
        } else {
            ++it;
        }
    }
}


void Game::SceneManager::run_interaction_handlers() {
    constexpr float INTERACTION_DISTANCE = 8.0f;
    const Uint8*    keys                 = SDL_GetKeyboardState(nullptr);
    if (!game_state->closest_model.empty()) {
        SDL_Event ev;
        SDL_PollEvent(&ev);
        if (game_state->distance_from_closest_model < INTERACTION_DISTANCE) {
            if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0 && keys[SDL_SCANCODE_I]) {
                run_handler_for(game_state->closest_model);
            }
            bottom_text_hints = "Interact with " + game_state->closest_model + " (Press I)";
        } else {
            bottom_text_hints = "";
        }
    }
}
void Game::SceneManager::handle_sdl_events(bool& running) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            terminate_game("Quitting Game...");
        }

        if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            int w = ev.window.data1, h = ev.window.data2;
            camera.set_window(w, h);
            set_viewport(0, 0, w, h);
        }

        const auto keys = SDL_GetKeyboardState(nullptr);
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0 && keys[SDL_SCANCODE_M]) {
            std::cout << "Position: " << camera.get_position().x << "," << camera.get_position().y
                      << "," << camera.get_position().z << "\n";
        }
        // feed mouse/window events to the camera
        camera.process_input(ev);
    }
}

void Game::SceneManager::perform_culling() {
    auto frustum_planes  = camera.extract_frustum_planes();
    auto camera_position = camera.get_position();

    for (auto& model : game_state->get_models()) {
        model->in_frustum(frustum_planes);
    }
}

void Game::SceneManager::check_collisions(float dt) {
    // Reset at the start of each loop
    game_state->distance_from_closest_model = std::numeric_limits<float>::max();

    auto monster_model = game_state->find_model("monster");
    if (!monster_model) {
        throw std::runtime_error("Could not find model monster when doing collision testing...");
    }

    // Precompute monster world‐space center
    monster_model->update_world_transform(glm::mat4(1.0f));
    glm::vec3 monster_center = 0.5f * (monster_model->get_aabbmin() + monster_model->get_aabbmax());

    const auto camera_pos     = camera.get_position();
    const auto camera_radius  = camera.get_radius();
    const auto monster_name   = monster_model->name();
    const auto last_mon_xform = last_monster_transform;

    const float monster_sphere_radius       = 0.8f;
    auto        cameraCollidesWithModelAABB = [&](Models::Model* model) {
        // game wise it might be more fun if it can go through walls
        bool monster_collision_enabled = false;
        // update “closest interactable” tracking
        if (model->can_interact()) {
            float distance_from_model = model->distance_from_point_using_AABB(camera_pos);
            if (distance_from_model < game_state->distance_from_closest_model) {
                // unnecessary copy here
                game_state->closest_model               = model->name();
                game_state->distance_from_closest_model = distance_from_model;
            }
        }

        // camera–AABB collision
        auto is_collided = model->intersect_sphere_aabb(camera_pos, camera_radius);
        // game logic
        if (is_collided && model->name() == "monster") {
            terminate_game("You died");
            return false;
        }
        if (is_collided) {
            // camera.set_position(last_cam_pos);
            return true;
        }

        // TODO enable monster collisions in some way outside of this method
        // monster–AABB collision (skip self)
        //  if (monster_collision_enabled) {
        //      auto monster_is_collided =
        //          model->intersect_sphere_aabb(monster_center, monster_sphere_radius);
        //      if (model->name() != monster_name && monster_is_collided) {
        //          // std::cout << "Name is: " << name << ", monster name: " << monster_name <<
        //          "\n"; monster_model->set_local_transform(last_mon_xform); return true;
        //      }
        //  }
        return false;
    };

    // Iterate models; break out of both loops if a collision happens.
    bool collision_detected = false;
    for (auto& model : game_state->get_models()) {
        if (!model->is_active())
            continue;
        if (cameraCollidesWithModelAABB(model.get())) {
            camera.set_position(last_camera_position);
            break;
        }
    }
}

void Game::SceneManager::render(const glm::mat4& view, const glm::mat4& projection) {
    renderer.render(view, projection,  game_state->get_lights(),  game_state->get_models());


    glm::mat4   textProjection = glm::ortho(0.0f, (float)screen_width, 0.0f, (float)screen_height);
    std::string displayed_text  = "pages:" + std::to_string(game_state->pages_collected);
        renderer.renderText(displayed_text, 50.0f, 720.0f - 50.0f, 1.2f,
                              {1.0f, 0.0f, 0.0f}, textProjection);

    if (!center_text.empty()) {
        renderer.renderText(center_text, 1280.0f / 2 - 80.0f, 720.0f - 250.0f,
                                  1.2f, {1.0f, 0.0f, 0.0f}, textProjection);
    }

    if (!bottom_text_hints.empty()) {
        renderer.renderText(bottom_text_hints, 300.0f, 720.0f - 650.0f, 0.8f,
                                  {0.5f, 0.5f, 0.0f}, textProjection);
    }else {
            renderer.renderText(std::to_string(fps),300.0f,720.0f - 650.0f,0.8f,{0.5f,0.5f,0.0f},textProjection);
    }
}

Game::SceneManager::SceneManager(int width, int height, glm::vec3 camera_position)
    : screen_width(width), screen_height(height), camera(width, height, camera_position) {
    event_handlers = {};
    initialise_opengl_sdl();
}

void Game::SceneManager::set_game_state(Game::GameState& g) {
    game_state = &g;
}

void Game::SceneManager::allocate_game_state_to_gpu() {
    assert(game_state);
    for(auto const& light: game_state->get_lights()){
        renderer.upload_light(light.get());
    }

    for(auto const& model: game_state->get_models()){
        renderer.upload_model(model.get());
    }
}

Game::SceneManager::~SceneManager() {
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    SDL_Quit();
}
