#include "SceneManager.h"
#include "Camera.h"
#include "Light.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL_keyboard.h>
#include <SDL_timer.h>
#include <algorithm>
#include <chrono>
#include <glm/gtx/vector_angle.hpp>
#include <iostream>
#include <random>

void Game::GameState::add_model(std::unique_ptr<Models::Model> model, const std::string& name) {
    size_t idx          = models.size();
    model_indices[name] = idx;
    model_names.push_back(name);
    models.push_back(std::move(model));
}

void Game::GameState::add_model(Models::Model&& model, const std::string& name) {
    // construct the heap‐object by moving the caller’s model in
    auto ptr = std::make_unique<Models::Model>(std::move(model));
    // and then register exactly as before:
    size_t idx          = models.size();
    model_indices[name] = idx;
    model_names.push_back(std::move(name));
    models.push_back(std::move(ptr));
}

void Game::GameState::remove_model(const std::string& name) {
    auto it = model_indices.find(name);
    if (it == model_indices.end())
        return;

    size_t idx  = it->second;
    size_t last = models.size() - 1;

    if (idx != last) {
        std::swap(models[idx], models[last]);
        std::swap(model_names[idx], model_names[last]);
        model_indices[model_names[idx]] = idx;
    }
    models.pop_back();
    model_names.pop_back();
    model_indices.erase(it);
}

Models::Model* Game::GameState::find_model(const std::string& name) const {
    auto it = model_indices.find(name);
    if (it == model_indices.end())
        return nullptr;
    return models[it->second].get();
}

const std::vector<std::unique_ptr<Models::Model>>& Game::GameState::get_models() const {
    return models;
}

void Game::GameState::add_light(std::unique_ptr<Light> light, const std::string& name) {
    size_t idx          = lights.size();
    light_indices[name] = idx;
    light_names.push_back(name);
    lights.push_back(std::move(light));
}

void Game::GameState::add_light(Light&& light, const std::string& name) {
    // construct the heap‐object by moving the caller’s model in
    auto ptr = std::make_unique<Light>(std::move(light));
    // and then register exactly as before:
    size_t idx          = lights.size();
    light_indices[name] = idx;
    light_names.push_back(std::move(name));
    lights.push_back(std::move(ptr));
}

void Game::GameState::remove_light(const std::string& name) {
    auto it = light_indices.find(name);
    if (it == light_indices.end())
        return;

    size_t idx  = it->second;
    size_t last = lights.size() - 1;

    if (idx != last) {
        std::swap(lights[idx], lights[last]);
        std::swap(light_names[idx], light_names[last]);
        light_indices[light_names[idx]] = idx;
    }
    lights.pop_back();
    light_names.pop_back();
    light_indices.erase(it);
}

Light* Game::GameState::find_light(const std::string& name) const {
    auto it = light_indices.find(name);
    if (it == light_indices.end())
        return nullptr;
    return lights[it->second].get();
}

const std::vector<std::unique_ptr<Light>>& Game::GameState::get_lights() const {
    return lights;
}

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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // SDL_Window*
    window = SDL_CreateWindow("Old room", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    // SDL_GLContext
    glCtx = SDL_GL_CreateContext(window);

    glewInit();
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 1280, 720);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void Game::SceneManager::initialise_shaders() {
    std::vector<std::string> shader_paths = {"assets/shaders/blinnphong.vert",
                                             "assets/shaders/blinnphong.frag"};
    std::vector<GLenum>      shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto blinnphong = std::make_shared<Shader>(shader_paths, shader_types, "blinn-phong");

    shader_paths  = {"assets/shaders/depth_2d.vert", "assets/shaders/depth_2d.frag"};
    shader_types  = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto depth_2d = std::make_shared<Shader>(shader_paths, shader_types, "depth_2d");

    shader_paths    = {"assets/shaders/depth_cube.vert", "assets/shaders/depth_cube.geom",
                       "assets/shaders/depth_cube.frag"};
    shader_types    = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
    auto depth_cube = std::make_shared<Shader>(shader_paths, shader_types, "depth_cube");

    shader_paths    = {"assets/shaders/text.vert", "assets/shaders/text.frag"};
    shader_types    = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto textshader = std::make_shared<Shader>(shader_paths, shader_types, "text");

    add_shader(blinnphong);
    add_shader(depth_2d);
    add_shader(depth_cube);
    add_shader(textshader);
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

    text_renderer.load_font("assets/fonts/scary.ttf");

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
            Mix_PlayMusic(horror_music, -1);// -1 = loop forever
            Mix_VolumeMusic(MIX_MAX_VOLUME/4);
            Mix_PlayChannel(footsteps_sound_channel, footsteps_sound, -1);
        }
    });

    monster.on_monster_not_active([footsteps_sound_channel](auto m) {
        if (Mix_PlayingMusic()) {
            Mix_HaltMusic();
            Mix_HaltChannel(footsteps_sound_channel);
        }
    });
    monster.on_chase_start([](){
        if(Mix_PlayingMusic()){
            Mix_VolumeMusic(MIX_MAX_VOLUME);
        }
    });
    monster.on_chase_stop([](){
        if(Mix_PlayingMusic()){
            Mix_VolumeMusic(MIX_MAX_VOLUME/4);
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
        lastTicks  = now;
        handle_sdl_events(running);
        last_camera_position   = camera.get_position();
        last_monster_transform = monster.monster_model()->get_local_transform();
        camera.update(dt);
        // std::cout << "Last camera position and current: \n";
        auto camera_dir = glm::normalize(camera.get_direction());
        monster.update(dt, camera_dir, last_camera_position);
        if (monster.monster_model()->is_active()) {
            glm::vec3 cam_pos = camera.get_position();
            glm::mat4 mon_tf  = monster.monster_model()->get_local_transform();
            glm::vec3 mon_pos = glm::vec3(mon_tf[3]);
            glm::vec3 to_monster = mon_pos - cam_pos;
            glm::vec3 dir_xz     = glm::normalize(glm::vec3(to_monster.x, 0.0f, to_monster.z));
            glm::vec3 forward    = camera.get_direction();
            glm::vec3 forward_xz = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));

            float angle_rad = glm::orientedAngle(forward_xz, dir_xz, glm::vec3(0.0f, 1.0f, 0.0f));
            float angle_deg = glm::degrees(angle_rad);

            float distance_f = glm::length(to_monster);
            distance_f = std::clamp(distance_f, 0.0f, 255.0f);

            uint8_t distance_byte = static_cast<uint8_t>(distance_f + 0.5f);
            Mix_SetPosition(footsteps_sound_channel, angle_deg, distance_f);
        }
       

        check_collisions(dt);
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // glEnable(GL_BLEND);
        // glBlendFunc(GL_ONE,GL_ONE);

        float     right_offset = 0.4f;
        glm::vec3 offset =
            right_offset * camera.get_right(); // + forward_offset * camera.get_direction()
        flashlight->set_position(camera.get_position() + offset);
        flashlight->set_direction(camera.get_direction());

        render_depth_pass();
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 proj = camera.get_projection_matrix();
        perform_culling();
        render(view, proj);
        run_interaction_handlers();
        SDL_GL_SwapWindow(window);
    }
    Mix_FreeChunk(footsteps_sound);
    Mix_FreeMusic(horror_music);
    SDL_Delay(seconds_to_wait_before_termination * 1000);
}

std::shared_ptr<Shader> Game::SceneManager::get_shader_by_name(const std::string& shader_name) {

    auto shaderPos =
        std::find_if(shaders.begin(), shaders.end(), [shader_name](std::shared_ptr<Shader> s) {
            return s->get_shader_name() == shader_name;
        });

    assert(shaderPos != shaders.end());
    return *shaderPos;
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

void Game::SceneManager::remove_instanced_model_at(const std::string& name,
                                                   const std::string& suffix) {
    // assumes impl detail that instance names are created by model->name() + suffix
    auto it = event_handlers.find(name + suffix);
    if (it != event_handlers.end()) {
        game_state->find_model(name)->remove_instance_transform(suffix);
    }
}

void Game::SceneManager::remove_model(const std::string& name) {
    auto it = event_handlers.find(name);
    if (it != event_handlers.end()) {
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

void Game::SceneManager::render_depth_pass() {
    auto depth2D   = get_shader_by_name("depth_2d");
    auto depthCube = get_shader_by_name("depth_cube");

    for (auto& light : game_state->get_lights()) {
        if(!light->is_turned_on()){
            continue;
        } 
        std::shared_ptr<Shader> sh;
        if (light->get_type() == LightType::POINT) {
            auto depthCube = get_shader_by_name("depth_cube");
            sh             = depthCube;
        } else {
            auto depth2D = get_shader_by_name("depth_2d");
            sh           = depth2D;
        }
        light->draw_depth_pass(sh, game_state->get_models());
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
            glViewport(0, 0, w, h);
        }
        
        const auto keys = SDL_GetKeyboardState(nullptr);
        if(ev.type == SDL_KEYDOWN && ev.key.repeat == 0 && keys[SDL_SCANCODE_M]){
            std::cout << "Position: " << camera.get_position().x << "," << camera.get_position().y << "," << camera.get_position().z << "\n";
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
    const auto last_cam_pos   = last_camera_position;
    const auto last_mon_xform = last_monster_transform;

    const float monster_sphere_radius = 0.8f;
    // A small helper to handle distance update and both camera/monster collision.
    // Returns true if any collision occured, so we can break out.
    auto processAABB = [&](Models::Model* model) {
        std::string name             = "";
        bool        closer           = false;
        float       squared_distance = 0.0f;
        // game wise it might be more fun if it can go through walls
        bool monster_collision_enabled = false;
        // update “closest interactable” tracking
        if (model->can_interact()) {
            std::tie(name, closer, squared_distance) = model->is_closer_than_current_model(
                camera_pos, game_state->distance_from_closest_model);
            if (closer) {
                game_state->closest_model               = name;
                game_state->distance_from_closest_model = squared_distance;
            }
        }

        // camera–AABB collision
        auto [is_collided, instance_index_camera] =
            model->intersect_sphere_aabb(camera_pos, camera_radius);
        // game logic
        if (is_collided && model->name() == "monster") {
            terminate_game("You died");
            return false;
        }
        if (is_collided) {
            camera.set_position(last_cam_pos);
            return true;
        }

        // monster–AABB collision (skip self)
        if (monster_collision_enabled) {
            auto [monster_is_collided, instance_index_monster] =
                model->intersect_sphere_aabb(monster_center, monster_sphere_radius);
            if (model->name() != monster_name && monster_is_collided) {
                // std::cout << "Name is: " << name << ", monster name: " << monster_name << "\n";
                monster_model->set_local_transform(last_mon_xform);
                return true;
            }
        }
        return false;
    };

    // Iterate models; break out of both loops if a collision happens.
    bool collision_detected = false;
    for (auto& model : game_state->get_models()) {
        if (!model->is_active())
            continue;
        if (processAABB(model.get()))
            break;
    }
}

void Game::SceneManager::render(const glm::mat4& view, const glm::mat4& projection) {

    GLCall(glViewport(0, 0, screen_width, screen_height));
    GLCall(glEnable(GL_MULTISAMPLE));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    auto shader = get_shader_by_name("blinn-phong");

    shader->use();
    shader->set_int("numLights", (GLint)game_state->get_lights().size());

    for (size_t i = 0; i < game_state->get_lights().size(); ++i) {
        auto        light = game_state->get_lights()[i].get();
        std::string base  = "lights[" + std::to_string(i) + "].";
        light->bind_shadow_map(shader, base, i);
        light->draw_lighting(shader, base, i);
    }

    for (auto const& model : game_state->get_models()) {
        if (!model->is_active()) {
            continue;
        }
        
        //TODO this has a problem, in the beginning not all things are rendered properly
        if (!model->is_in_frustum()) {
            // std::cout << model->name() << std::endl;
            continue;
        }

        model->update_world_transform(glm::mat4(1.0f));
        model->draw(view, projection, shader);
    }

    GLCall(glDisable(GL_MULTISAMPLE));
    glUseProgram(0);

    GLCall(glViewport(0, 0, screen_width, screen_height));
    glm::mat4   text_projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f);
    auto        textShader      = get_shader_by_name("text");
    std::string displayed_text  = "pages:" + std::to_string(game_state->pages_collected);
    text_renderer.render_text(textShader, displayed_text, 50.0f, 720.0f - 50.0f, 1.2f,
                              {1.0f, 0.0f, 0.0f}, text_projection);
    if (!center_text.empty()) {
        text_renderer.render_text(textShader, center_text, 1280.0f / 2 - 80.0f, 720.0f - 250.0f,
                                  1.2f, {1.0f, 0.0f, 0.0f}, text_projection);
    }

    if (!bottom_text_hints.empty()) {
        text_renderer.render_text(textShader, bottom_text_hints, 300.0f, 720.0f - 650.0f, 0.8f,
                                  {0.5f, 0.5f, 0.0f}, text_projection);
    }
    glUseProgram(0);
}

Game::SceneManager::SceneManager(int width, int height, Camera::CameraObj camera)
    : screen_width(width), screen_height(height), camera(camera) {
    event_handlers = {};
}

Game::SceneManager::~SceneManager() {
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    SDL_Quit();
    shaders.clear();
    // clearing game_state causes a double free
    // game_state->clear_models();
    // game_state->clear_lights();
}
