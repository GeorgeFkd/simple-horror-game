#include "SceneManager.h"
#include "Camera.h"
#include "Light.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL_timer.h>
#include <algorithm>
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

void Game::SceneManager::run_game_loop() {

    std::cout << "Attempting to load font";
    textRenderer.load_font();

    Mix_Music* horrorMusic = Mix_LoadMUS("assets/audio/scary.mp3");
    if (!horrorMusic) {
        std::cerr << "Failed to load music: " << Mix_GetError() << std::endl;
        return;
    }

    Mix_Chunk* footstepsMusic = Mix_LoadWAV("assets/audio/footsteps.mp3");

    float monster_follow_distance                          = 10.0f;
    float monster_follow_speed                             = 7.5f;
    float monster_time_looking_at_it                       = 0.0f;
    float monster_time_not_looking_at_it                   = 0.0f;
    int   monster_seconds_per_coinflip                     = 20;
    float monster_random_dissapear_probability             = 0.35f;
    float monster_dissapear_probability_when_looking_at_it = 0.75f;
    float monster_seconds_to_look_at_it_for_coinflip       = 5.0f;
    float monster_seconds_looking_at_it_for_death          = 7.0f;
    float monster_seconds_not_looking_at_it_for_death      = 1000.0f;
    // glm::dot(camera_dir,monster), -1 looking away from monster, 1 looking it directly
    float monster_view_dir         = 0.0f;
    auto  monster_initial_position = glm::vec3(5.0f, 0.0f, 5.0f);
    last_monster_transform         = glm::translate(glm::mat4(1.0f), monster_initial_position);
    auto monster_init              = Models::Model("assets/models/monster.obj", "monster");

    game_state->add_model(std::move(monster_init), "monster");

    auto monster = game_state->find_model("monster");

    if (!monster) {
        throw std::runtime_error("Could not find monster model...");
    }

    monster->set_local_transform(last_monster_transform);
    monster->update_world_transform(glm::mat4(1.0f));
    std::random_device               r;
    std::default_random_engine       el;
    std::uniform_real_distribution<> uniform_rand(0, 1);
    bool                             running             = true;
    Uint64                           lastTicks           = SDL_GetPerformanceCounter();
    int                              interactionDistance = 2.0f;
    auto                             flashlight          = game_state->find_light("flashlight");

    if (!flashlight) {
        throw std::runtime_error("Could not find flashlight model...");
    }

    float elapsedTime = 0.0f;
    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float  dt  = float(now - lastTicks) / float(SDL_GetPerformanceFrequency());
        elapsedTime += dt;
        if (elapsedTime > monster_seconds_per_coinflip * 1.0f) {
            float randNum = uniform_rand(el);
            float probs   = 0.0f;
            if (monster->is_active()) {
                probs = monster_random_dissapear_probability;
            } else {
                probs = 1 - monster_random_dissapear_probability;
            }
            if (randNum < probs) {
                monster->toggle_active();
            }
            elapsedTime -= monster_seconds_per_coinflip * 1.0f;
        }

        lastTicks = now;
        handle_sdl_events(running);
        last_camera_position   = camera.get_position();
        last_monster_transform = monster->get_local_transform();
        camera.update(dt);
        // std::cout << "Last camera position and current: \n";
        auto camera_dir = glm::normalize(camera.get_direction());
        auto towards_monster =
            glm::normalize(glm::vec3(last_monster_transform[3]) - camera.get_position());
        monster_view_dir = glm::dot(camera_dir, towards_monster);
        // std::cout << "View with monster is: " << monster_view_dir << "\n";
        if (monster->is_active()) {
            // std::cout << "Time is ticking\n";
            if (monster_view_dir > 0) {
                monster_time_looking_at_it += dt;
                monster_time_not_looking_at_it = 0;
            } else {
                monster_time_not_looking_at_it += dt;
                monster_time_looking_at_it = 0;
            }
            if (monster_time_looking_at_it > monster_seconds_looking_at_it_for_death) {
                float randNum = uniform_rand(el);
                if (randNum < 0.65f) {
                    std::cout << "You died cause you looked at it too long\n";
                    flashlight->set_turned_on(false);
                    running = false;
                }
            }

            if (monster_time_not_looking_at_it > monster_seconds_not_looking_at_it_for_death) {
                float randNum = uniform_rand(el);
                if (randNum < 0.65f) {
                    std::cout << "You died cause you didn't look at it\n";
                    flashlight->set_turned_on(false);
                    running = false;
                }
            }

            if (monster_time_looking_at_it > monster_seconds_to_look_at_it_for_coinflip * 1.0f) {
                float randNum = uniform_rand(el);
                if (randNum < 0.25f) {
                    monster->toggle_active();
                }
            }
        } else {
            // everytime it dissappears reset these
            monster_time_looking_at_it     = 0.0f;
            monster_time_not_looking_at_it = 0.0f;
        }
        if (monster->is_active()) {
            if (Mix_PlayingMusic() == 0) {
                Mix_PlayMusic(horrorMusic, -1);          // -1 = loop forever
                Mix_PlayChannel(-1, footstepsMusic, -1); //-1 as channel means that sdl allocates it
            }

        } else {
            if (Mix_PlayingMusic()) {
                Mix_HaltMusic();
                Mix_HaltChannel(-1);
            }
        }
        // if you dont move the monster does not move, but can dissappear
        if (last_camera_position != camera.get_position()) {
            auto target_pos      = camera.get_position() - camera_dir * monster_follow_distance;
            auto new_monster_pos = glm::mix(glm::vec3(last_monster_transform[3]), target_pos,
                                            monster_follow_speed * dt);
            new_monster_pos.y    = 0.0f;
            auto  forward        = glm::normalize(glm::vec3(camera_dir.x, 0.0f, camera_dir.z));
            float angle          = glm::atan(forward.x, forward.z);

            auto tf = glm::translate(glm::mat4(1.0f), new_monster_pos);
            tf      = glm::rotate(tf, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            monster->set_local_transform(tf);
        }

        // loop over models(collision tests, update closest object
        check_all_models(dt);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
        render(view, proj);
        run_interaction_handlers();
        SDL_GL_SwapWindow(window);
    }
    Mix_FreeChunk(footstepsMusic);
    Mix_FreeMusic(horrorMusic);
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
        std::shared_ptr<Shader> sh;
        if (light->get_type() == LightType::POINT) {
            sh = depthCube;
        } else {
            sh = depth2D;
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
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0 && keys[SDL_SCANCODE_I]) {
            if (game_state->distance_from_closest_model < INTERACTION_DISTANCE) {
                std::cout << "Running handler for: " << game_state->closest_model << "\n";
                run_handler_for(game_state->closest_model);
            }
        }
    }
}

void Game::SceneManager::handle_sdl_events(bool& running) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            running = false;
        }
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
            // interaction handlers run after the render
        }
        // TODO(optional) fix resizing
        if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            int w = ev.window.data1, h = ev.window.data2;
            glViewport(0, 0, w, h);
        }
        // feed mouse/window events to the camera
        camera.process_input(ev);
        // adjust the GL viewport on resize
    }
}

void Game::SceneManager::check_all_models(float dt) {
    // Reset at the start of each loop
    game_state->distance_from_closest_model = std::numeric_limits<float>::max();

    auto monster = game_state->find_model("monster");
    if (!monster) {
        throw std::runtime_error("Could not find model monster...");
    }

    // Precompute monster world‐space center
    monster->update_world_transform(glm::mat4(1.0f));
    glm::vec3 monster_center = 0.5f * (monster->get_aabbmin() + monster->get_aabbmax());

    const auto camera_pos     = camera.get_position();
    const auto camera_radius  = camera.get_radius();
    const auto monster_name   = monster->name();
    const auto last_cam_pos   = last_camera_position;
    const auto last_mon_xform = last_monster_transform;

    const float monster_sphere_radius = 0.8f;
    // A small helper to handle distance update and both camera/monster collision.
    // Returns true if any collision occured, so we can break out.
    auto processAABB = [&](Models::Model* model) {
        std::string name             = "";
        bool        closer           = false;
        float       squared_distance = 0.0f;
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
        if (is_collided) {
            camera.set_position(last_cam_pos);
            return true;
        }

        // monster–AABB collision (skip self)
        auto [monster_is_collided, instance_index_monster] =
            model->intersect_sphere_aabb(monster_center, monster_sphere_radius);
        if (name != monster_name && monster_is_collided) {
            monster->set_local_transform(last_mon_xform);
            return true;
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
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    auto shader = get_shader_by_name("blinn-phong");

    shader->use();
    shader->set_int("numLights", (GLint)game_state->get_lights().size());

    for (size_t i = 0; i < game_state->get_lights().size(); ++i) {
        auto        light = game_state->get_lights()[i].get();
        std::string base  = "lights[" + std::to_string(i) + "].";
        light->draw_lighting(shader, base, i);
        light->bind_shadow_map(shader, base, i);
    }

    for (auto const& model : game_state->get_models()) {
        if (model->is_active()) {
            model->update_world_transform(glm::mat4(1.0f));
            // instancing is an impl detail
            model->draw(view, projection, shader);
        }
    }

    glUseProgram(0);

    GLCall(glViewport(0, 0, screen_width, screen_height));
    glm::mat4   text_projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f);
    auto        textShader      = get_shader_by_name("text");
    std::string displayed_text  = "pages:" + std::to_string(game_state->pages_collected);
    textRenderer.RenderText(textShader, displayed_text, 50.0f, 720.0f - 50.0f, 1.2f,
                            {1.0f, 0.0f, 0.0f}, text_projection);
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
    game_state->clear_models();
    game_state->clear_lights();
}
