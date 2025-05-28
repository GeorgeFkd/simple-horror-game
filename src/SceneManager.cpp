#include "SceneManager.h"
#include "Camera.h"
#include <SDL_timer.h>
#include <algorithm>
#include <iostream>
#include <random>
// GameState methods
Models::Model* Game::GameState::findModel(Models::Model* model) {
    auto model_pos = std::find_if(models.begin(), models.end(),
                                  [model](auto* m) { return m->name() == model->name(); });
    return *model_pos;
}
Models::Model* Game::GameState::findModel(std::string_view name) {
    auto model_pos =
        std::find_if(models.begin(), models.end(), [name](auto* m) { return m->name() == name; });
    return *model_pos;
}
void Game::GameState::remove_model(Models::Model* model) {
    // we probably need to switch to a hashmap, this costs O(n)
    auto res = std::remove_if(models.begin(), models.end(),
                              [model](auto* m) { return m->name() == model->name(); });
    models.erase(res, models.end());
}
Light* Game::GameState::findLight(std::string_view name) {
    auto light_pos =
        std::find_if(lights.begin(), lights.end(), [name](auto* l) { return l->name() == name; });
    return *light_pos;
}

// Scene Manager methods
void Game::SceneManager::initialise_opengl_sdl() {
    SDL_Init(SDL_INIT_VIDEO);
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

void Game::SceneManager::run_game_loop() {
#define printv3(v) std::cout << v.x << "," << v.y << "," << v.z << "\n";

#ifdef DEBUG_DEPTH
    shader_paths       = {"assets/shaders/depth_debug.vert", "assets/shaders/depth_debug.frag"};
    shader_types       = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
#endif

    float monster_follow_distance                          = 10.0f;
    float monster_follow_speed                             = 7.5f;
    float monster_time_looking_at_it                       = 0.0f;
    float monster_time_not_looking_at_it                   = 0.0f;
    int   monster_seconds_per_coinflip                     = 20;
    float monster_random_dissapear_probability             = 0.35f;
    float monster_dissapear_probability_when_looking_at_it = 0.75f;
    float monster_seconds_to_look_at_it_for_coinflip       = 5.0f;
    float monster_seconds_looking_at_it_for_death          = 7.0f;
    float monster_seconds_not_looking_at_it_for_death      = 10.0f;
    // glm::dot(camera_dir,monster), -1 looking away from monster, 1 looking it directly
    float monster_view_dir         = 0.0f;
    auto  monster_initial_position = glm::vec3(5.0f, 0.0f, 5.0f);
    last_monster_transform         = glm::translate(glm::mat4(1.0f), monster_initial_position);
    auto monster_init              = Models::Model("assets/models/monster.obj", "monster");
    std::cout << "Run game loop runs\n";
    gameState.add_model(monster_init);
    monster = gameState.findModel("monster");
    monster->set_local_transform(last_monster_transform);
    monster->update_world_transform(glm::mat4(1.0f));
    std::random_device               r;
    std::default_random_engine       el;
    std::uniform_real_distribution<> uniform_rand(0, 1);
    bool                             running             = true;
    Uint64                           lastTicks           = SDL_GetPerformanceCounter();
    int                              interactionDistance = 2.0f;
    auto                             flashlight          = gameState.findLight("flashlight");
    if (!flashlight) {
        std::cout << "Light with name: " << "flashlight" << " was not found\n";
        assert(false);
    }
    float elapsedTime = 0.0f;
    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float  dt  = float(now - lastTicks) / float(SDL_GetPerformanceFrequency());
        elapsedTime += dt;
        if (elapsedTime > monster_seconds_per_coinflip * 1.0f) {
            float randNum = uniform_rand(el);
            float probs   = 0.0f;
            if (monster->isActive()) {
                probs = monster_random_dissapear_probability;
            } else {
                probs = 1 - monster_random_dissapear_probability;
            }
            if (randNum < probs) {
                monster->toggleActive();
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
        std::cout << "View with monster is: " << monster_view_dir << "\n";
        if (monster->isActive()) {
            std::cout << "Time is ticking\n";
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
                    monster->toggleActive();
                }
            }
        } else {
            // everytime it dissappears reset these
            monster_time_looking_at_it     = 0.0f;
            monster_time_not_looking_at_it = 0.0f;
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
#ifdef DEBUG_DEPTH
        depth_debug.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flashlight.get_depth_texture());
        glUniform1i(depth_debug.get_uniform_location("depthMap"), 0);
        glUniform1f(depth_debug.get_uniform_location("near_plane"), 1.0f);
        glUniform1f(depth_debug.get_uniform_location("far_plane"), 100.0f);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
#endif

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
}

std::shared_ptr<Shader> Game::SceneManager::get_shader_by_name(const std::string& shader_name) {

    auto shaderPos =
        std::find_if(shaders.begin(), shaders.end(), [shader_name](std::shared_ptr<Shader> s) {
            return s->get_shader_name() == shader_name;
        });

    assert(shaderPos != shaders.end());
    return *shaderPos;
}
void Game::SceneManager::debug_dump_model_names() {
    for (auto m : gameState.get_models()) {
        std::cout << "Model: " << m->name() << "\n";
    }
}

void Game::SceneManager::move_model(Models::Model* model, const glm::vec3& direction) {
    auto model_pos = gameState.findModel(model);
    model_pos->move_relative_to(direction);
}

void Game::SceneManager::move_model(std::string_view name, const glm::vec3& direction) {
    auto model_pos = gameState.findModel(name);
    model_pos->move_relative_to(direction);
}

void Game::SceneManager::move_model_X(std::string_view name, float x) {
    move_model(name, glm::vec3(x, 0.0f, 0.0f));
}

void Game::SceneManager::move_model_Y(std::string_view name, float y) {
    move_model(name, glm::vec3(0.0f, y, 0.0f));
}

void Game::SceneManager::move_model_Z(std::string_view name, float z) {
    move_model(name, glm::vec3(0.0f, 0.0f, z));
}

void Game::SceneManager::remove_instanced_model_at(Models::Model*     model,
                                                   const std::string& suffix) {
    // assumes impl detail that instance names are created by model->name() + suffix
    auto it = eventHandlers.find(model->name() + suffix);
    if (it != eventHandlers.end()) {
        eventHandlers.erase(it);
        model->remove_instance_transform(suffix);
    }
}

void Game::SceneManager::remove_model(Models::Model* m) {
    auto it = eventHandlers.find(m->name());
    if (it != eventHandlers.end()) {
        eventHandlers.erase(it);
        gameState.remove_model(m);
    }
}

int Game::SceneManager::on_interaction_with(std::string_view                   instanceName,
                                            std::function<void(SceneManager*)> handler) {
    std::string keyStr(instanceName); // Make a copy to store
    eventHandlers.insert({keyStr, handler});
    return 0;
}

void Game::SceneManager::run_handler_for(const std::string& m) {
    std::cout << "Keys: " << eventHandlers.count(m) << "\n";
    std::cout << m << "\n";
    auto it = eventHandlers.find(m);
    if (it != eventHandlers.end()) {
        it->second(this);
    } else {
        std::cerr << "No handler for instance: " << m << "\n";
    }
}

void Game::SceneManager::render_depth_pass() {
    auto depth2D   = get_shader_by_name("depth_2d");
    auto depthCube = get_shader_by_name("depth_cube");

    for (auto* light : gameState.get_lights()) {
        std::shared_ptr<Shader> sh;
        if (light->get_type() == LightType::POINT) {
            sh = depthCube;
        } else {
            sh = depth2D;
        }
        light->draw_depth_pass(sh, gameState.get_models());
    }
}

void Game::SceneManager::run_interaction_handlers() {
    constexpr float interactionDistance = 8.0f;
    const Uint8*    keys                = SDL_GetKeyboardState(nullptr);
    if (!gameState.closestModel.empty()) {
        if (keys[SDL_SCANCODE_I]) {
            // std::cout << "user is interacting with: " << gameState.closestModel << "\n";

            if (gameState.distanceFromClosestModel < interactionDistance) {
                run_handler_for(gameState.closestModel);
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
    // reset at the start of each loop
    gameState.distanceFromClosestModel = std::numeric_limits<float>::max();
    monster->update_world_transform(glm::mat4(1.0f));
    for (auto* model : gameState.get_models()) {
        if (!model->isActive())
            continue;
        auto monster_center = 0.5f * (monster->get_aabbmin() + monster->get_aabbmax());
        // std::cout << "Monster center is: " << monster_center.x << "," << monster_center.y << ","
        //           << monster_center.z << "\n";
        if (model->is_instanced()) {
            for (size_t i = 0; i < model->get_instance_count(); ++i) {
                if (model->can_interact()) {
                    float dist = camera.distance_from_camera_using_AABB(
                        camera.get_position(), model->get_instance_aabb_min(i),
                        model->get_instance_aabb_max(i));
                    if (dist < gameState.distanceFromClosestModel) {
                        gameState.closestModel             = model->instance_name(i);
                        gameState.distanceFromClosestModel = dist;
                    }
                }
                bool collision_detected = false;
                if (camera.intersect_sphere_aabb(camera.get_position(), camera.get_radius(),
                                                 model->get_instance_aabb_min(i),
                                                 model->get_instance_aabb_max(i))) {

                    camera.set_position(last_camera_position);
                    collision_detected = true;
                }

                if (Camera::intersects_sphere_aabb(monster_center, 1.0f,
                                                   model->get_instance_aabb_min(i),
                                                   model->get_instance_aabb_max(i))) {
                    monster->set_local_transform(last_monster_transform);
                    std::cout << "Monster is bumping into things\n";
                    collision_detected = true;
                }

                if (collision_detected) {
                    goto collision_done;
                }
            }
        } else {
            if (model->can_interact()) {
                float dist = camera.distance_from_camera_using_AABB(
                    camera.get_position(), model->get_aabbmin(), model->get_aabbmax());
                if (dist < gameState.distanceFromClosestModel) {
                    gameState.closestModel             = model->name();
                    gameState.distanceFromClosestModel = dist;
                }
            }
            // single AABB path
            //
            bool collision_detected = false;
            if (camera.intersect_sphere_aabb(camera.get_position(), camera.get_radius(),
                                             model->get_aabbmin(), model->get_aabbmax())) {
                // std::cout << "Non instanced Intersection\n";
                if (!model->name().empty()) {
                    // std::cout << "Collision with: " << model->name() << "\n";
                }
                camera.set_position(last_camera_position);
                collision_detected = true;
            }
            if (model->name() != monster->name()) {

                if (Camera::intersects_sphere_aabb(monster_center, 0.8f, model->get_aabbmin(),
                                                   model->get_aabbmax())) {
                    monster->set_local_transform(last_monster_transform);
                    std::cout << "Monster is bumping into " << model->name() << "\n";
                    collision_detected = true;
                }
                if (collision_detected) {
                    goto collision_done;
                }
            }
        }
    }
collision_done:;
}

void Game::SceneManager::initialise_shaders() {
    std::vector<std::string> shader_paths = {"assets/shaders/blinnphong.vert",
                                             "assets/shaders/blinnphong.frag"};
    std::vector<GLenum>      shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto blinnphong = std::make_shared<Shader>(shader_paths, shader_types, "blinn-phong");

    shader_paths  = {"assets/shaders/depth_2d.vert", "assets/shaders/depth_2d.frag"};
    shader_types  = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto depth_2d = std::make_shared<Shader>(shader_paths, shader_types, "depth_2d");

#ifdef DEBUG_DEPTH
    shader_paths       = {"assets/shaders/depth_debug.vert", "assets/shaders/depth_debug.frag"};
    shader_types       = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
#endif

    shader_paths    = {"assets/shaders/depth_cube.vert", "assets/shaders/depth_cube.geom",
                       "assets/shaders/depth_cube.frag"};
    shader_types    = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
    auto depth_cube = std::make_shared<Shader>(shader_paths, shader_types, "depth_cube");

    add_shader(blinnphong);
    add_shader(depth_2d);
    add_shader(depth_cube);
}

void Game::SceneManager::render(const glm::mat4& view, const glm::mat4& projection) {

    // Optional: reset viewport to screen size
    GLCall(glViewport(0, 0, screen_width, screen_height));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    auto shader = get_shader_by_name("blinn-phong");

    shader->use();
    shader->set_int("numLights", (GLint)gameState.get_lights().size());

    for (size_t i = 0; i < gameState.get_lights().size(); ++i) {
        const Light* light = gameState.get_lights()[i];
        std::string  base  = "lights[" + std::to_string(i) + "].";
        light->draw_lighting(shader, base, i);
        light->bind_shadow_map(shader, base, i);
    }

    for (auto const& model : gameState.get_models()) {
        if (model->isActive()) {
            model->update_world_transform(glm::mat4(1.0f));
            // instancing is an impl detail
            model->draw(view, projection, shader);
        }
    }

    glUseProgram(0);
}

Game::SceneManager::SceneManager(int width, int height, Camera::CameraObj camera)
    : screen_width(width), screen_height(height), camera(camera) {
    eventHandlers = {};
}

Game::SceneManager::~SceneManager() {
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    gameState.get_models().clear();
    shaders.clear();
    gameState.get_lights().clear();
}
