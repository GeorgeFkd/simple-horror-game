#include "SceneManager.h"
#include "Camera.h"
#include <SDL_timer.h>
#include <algorithm>
#include <iostream>
#include <tuple>
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
void Game::SceneManager::initialiseOpenGL_SDL() {
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

void Game::SceneManager::runGameLoop() {

#ifdef DEBUG_DEPTH
    shader_paths       = {"assets/shaders/depth_debug.vert", "assets/shaders/depth_debug.frag"};
    shader_types       = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
#endif
    bool   running             = true;
    Uint64 lastTicks           = SDL_GetPerformanceCounter();
    int    interactionDistance = 2.0f;
    // float  elapsedTime         = 0.0f;
    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float  dt  = float(now - lastTicks) / float(SDL_GetPerformanceFrequency());
        // elapsedTime += dt;
        // if (elapsedTime > 1.0f) {
        //     // std::cout << "This should run every 1second\n";
        //     elapsedTime -= 1.0f;
        // }
        lastTicks = now;
        handleSDLEvents(running);
        last_camera_position = camera.get_position();
        // loop over models(collision tests, update closest object
        camera.update(dt);
        checkAllModels(dt);
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
        auto flashlight = gameState.findLight("flashlight");
        if (!flashlight) {
            std::cout << "Light with name: " << "flashlight" << " was not found\n";
            assert(false);
        }
        float right_offset = 0.4f;
        glm::vec3 offset = right_offset * camera.get_right(); // + forward_offset * camera.get_direction()
        flashlight->set_position(camera.get_position() + offset);
        flashlight->set_direction(camera.get_direction());
        render_depth_pass();
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 proj = camera.get_projection_matrix();
        render(view, proj);
        runInteractionHandlers();
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

void Game::SceneManager::remove_instanced_model_at(Models::Model* model, size_t instancePosition) {
    model->remove_instance_transform(instancePosition);
    // auto key = std::make_tuple(model->name(),instancePosition);
    // eventHandlers.erase(key);
}

void Game::SceneManager::remove_model(Models::Model* m) {
    // auto key = std::make_tuple(m->name(),std::nullopt);
    // eventHandlers.erase(key);
    gameState.remove_model(m);
}


int Game::SceneManager::on_interaction_with_instance(
    std::string_view name, size_t instancePos, std::function<void(Game::SceneManager*)> handler) {
    std::cout << "Adding interaction rule for: " << name << " at instance: " << instancePos << "\n";
    eventHandlers.insert({std::make_tuple(name, instancePos), handler});
    return 0;
}

int Game::SceneManager::on_interaction_with(std::string_view                   name,
                                            std::function<void(SceneManager*)> handler) {
#if 1
    std::cout << name << "-> " << sizeof(handler) << "\n";
#endif
    eventHandlers.insert({std::make_tuple(name, std::nullopt), handler});
    return 0;
}

void Game::SceneManager::run_handler_for(const ModelInstance& m) {
    std::cout << "Run handler for: " << std::get<std::string_view>(m) << " at "
              << std::get<1>(m).value() << "\n";
    auto key = m;
    std::cout << "Keys: " << eventHandlers.count(key) << "\n";
    assert(eventHandlers.find(key) != eventHandlers.end());
    eventHandlers.at(key)(this);
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

void Game::SceneManager::runInteractionHandlers() {
    constexpr float interactionDistance = 5.0f;
    const Uint8*    keys                = SDL_GetKeyboardState(nullptr);
    auto            name                = std::get<std::string_view>(gameState.closestModel);
    if (!name.empty()) {
        if (keys[SDL_SCANCODE_I]) {
            std::cout << "user is interacting with: " << name
                      << "at: " << std::get<1>(gameState.closestModel).value() << "\n";
            if (gameState.distanceFromClosestModel < interactionDistance) {
                run_handler_for(gameState.closestModel);
            }
        }
    }
}

void Game::SceneManager::handleSDLEvents(bool& running) {
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

void Game::SceneManager::checkAllModels(float dt) {
    // reset at the start of each loop
    gameState.distanceFromClosestModel = std::numeric_limits<float>::max();
    for (auto* model : gameState.get_models()) {
        if (!model->isActive())
            continue;
        if (model->is_instanced()) {
            for (size_t i = 0; i < model->get_instance_count(); ++i) {
                if (model->can_interact()) {
                    float dist = camera.distanceFromCameraUsingAABB(
                        camera.get_position(), model->get_instance_aabb_min(i),
                        model->get_instance_aabb_max(i));
                    if (dist < gameState.distanceFromClosestModel) {
                        gameState.closestModel =
                            std::make_tuple(model->name(), std::optional<size_t>(i));
                        gameState.distanceFromClosestModel = dist;
                    }
                }
                if (camera.intersectSphereAABB(camera.get_position(), camera.get_radius(),
                                               model->get_instance_aabb_min(i),
                                               model->get_instance_aabb_max(i))) {
                    if (!model->name().empty()) {
                        std::cout << "Collision with: " << model->name() << " at:" << i << "\n";
                    }
                    camera.set_position(last_camera_position);
                    goto collision_done;
                }
            }
        } else {
            if (model->can_interact()) {
                float dist = camera.distanceFromCameraUsingAABB(
                    camera.get_position(), model->get_aabbmin(), model->get_aabbmax());
                if (dist < gameState.distanceFromClosestModel) {
                    gameState.closestModel = std::make_tuple(model->name(), std::nullopt);
                    gameState.distanceFromClosestModel = dist;
                }
            }
            // single AABB path
            if (camera.intersectSphereAABB(camera.get_position(), camera.get_radius(),
                                           model->get_aabbmin(), model->get_aabbmax())) {
                std::cout << "Non instanced Intersection\n";
                if (!model->name().empty()) {
                    std::cout << "Collision with: " << model->name() << "\n";
                }
                camera.set_position(last_camera_position);
                break;
            }
        }
    }
collision_done:;
}

void Game::SceneManager::initialiseShaders() {
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
