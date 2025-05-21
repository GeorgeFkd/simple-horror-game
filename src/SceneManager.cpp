#include "SceneManager.h"
#include "Camera.h"
#include <algorithm>
#include <iostream>

std::shared_ptr<Shader> Game::SceneManager::get_shader_by_name(const std::string& shader_name) {
    
    auto shaderPos = std::find_if(shaders.begin(),shaders.end(),[shader_name](std::shared_ptr<Shader> s){ return s->get_shader_name() == shader_name;});
    
    assert(shaderPos != shaders.end());
    return *shaderPos;
}
void Game::SceneManager::debug_dump_model_names() {
    for (auto m : gameState.models) {
        std::cout << "Model: " << m->name() << "\n";
    }
}

void Game::SceneManager::move_model(Models::Model* model, const glm::vec3& direction) {
    auto model_pos = findModel(model);
    model_pos->move_relative_to(direction);
}

void Game::SceneManager::move_model(std::string_view name, const glm::vec3& direction) {
    auto model_pos = findModel(name);
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
void Game::SceneManager::remove_model(Models::Model* model) {
    // we probably need to switch to a hashmap, this costs O(n)
    auto res = std::remove_if(gameState.models.begin(), gameState.models.end(),
                              [model](auto* m) { return m->name() == model->name(); });
    gameState.models.erase(res, gameState.models.end());
}

void Game::SceneManager::remove_instanced_model_at() {
    throw std::runtime_error("not yet implemented");
}
Models::Model* Game::SceneManager::findModel(Models::Model* model) {
    auto model_pos = std::find_if(gameState.models.begin(), gameState.models.end(),
                                  [model](auto* m) { return m->name() == model->name(); });
    return *model_pos;
}
Models::Model* Game::SceneManager::findModel(std::string_view name) {
    auto model_pos = std::find_if(gameState.models.begin(), gameState.models.end(),
                                  [name](auto* m) { return m->name() == name; });
    return *model_pos;
}

Light* Game::SceneManager::findLight(std::string_view name) {
    auto light_pos = std::find_if(gameState.lights.begin(), gameState.lights.end(),
                                  [name](auto* l) { return l->name() == name; });
    return *light_pos;
}
int Game::SceneManager::on_interaction_with(Models::Model*                     m,
                                            std::function<void(SceneManager*)> handler) {
#if 1
    std::cout << m->name() << "-> " << sizeof(handler) << "\n";
#endif

    eventHandlers[m->name()] = handler;
    return 0;
}

int Game::SceneManager::on_interaction_with(std::string_view                   name,
                                            std::function<void(SceneManager*)> handler) {
#if 1
    std::cout << name << "-> " << sizeof(handler) << "\n";
#endif
    eventHandlers.insert({name, handler});
    return 0;
}

int Game::SceneManager::run_handler_for(Models::Model* m) {
    std::cout << "Run handler for: " << m->name() << "\n";
    std::cout << "Keys: " << eventHandlers.count(m->name()) << "\n";
    assert(eventHandlers.find(m->name()) != eventHandlers.end());
    eventHandlers.at(m->name())(this);
    return 0;
}

void Game::SceneManager::run_handler_for(std::string_view name) {
    std::cout << "Run handler for: " << name << "\n";
    std::cout << "Keys: " << eventHandlers.count(name) << "\n";
    assert(eventHandlers.find(name) != eventHandlers.end());
    eventHandlers.at(name)(this);
}
void Game::SceneManager::render_depth_pass() {
    auto depth2D   = get_shader_by_name("depth_2d");
    auto depthCube = get_shader_by_name("depth_cube");

    for (auto* light : gameState.lights) {
        if (light->get_type() == LightType::POINT) {
            light->draw_depth_pass(depthCube, gameState.models);
        } else {
            light->draw_depth_pass(depth2D, gameState.models);
        }
    }
}

void Game::SceneManager::runInteractionHandlers() {
    constexpr float interactionDistance = 5.0f;
    const Uint8*    keys                = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_I]) {
        if (!gameState.closestModelName.empty() &&
            gameState.distanceFromClosestModel < interactionDistance) {
            run_handler_for(gameState.closestModelName);
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
            const Uint8* keys = SDL_GetKeyboardState(nullptr);
        }
        // feed mouse/window events to the camera
        camera.process_input(ev);
        // adjust the GL viewport on resize
        if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            int w = ev.window.data1, h = ev.window.data2;
            glViewport(0, 0, w, h);
        }
    }
}

void Game::SceneManager::run(float dt) {
    // reset at the start of each loop
    auto last_camera_position = camera.get_position();
    camera.update(dt);
    gameState.distanceFromClosestModel = std::numeric_limits<float>::max();
    for (auto* model : gameState.get_models()) {
        if (!model->isActive())
            continue;
        if (model->is_instanced()) {
            // loop each instance’s box
            for (size_t i = 0; i < model->get_instance_count(); ++i) {
                if (camera.intersectSphereAABB(camera.get_position(), camera.get_radius(),
                                               model->get_instance_aabb_min(i),
                                               model->get_instance_aabb_max(i))) {
                    std::cout << "Collision with: " << model->name() << " at:" << i << "\n";

                    camera.set_position(last_camera_position);
                    goto collision_done;
                }
            }
        } else {
            if (model->can_interact() && !model->is_instanced()) {
                float dist = camera.distanceFromCameraUsingAABB(
                    camera.get_position(), model->get_aabbmin(), model->get_aabbmax());
                if (dist < gameState.distanceFromClosestModel) {
                    gameState.closestModelName         = model->name();
                    gameState.distanceFromClosestModel = dist;
                }
            }
            // single AABB path
            if (camera.intersectSphereAABB(camera.get_position(), camera.get_radius(),
                                           model->get_aabbmin(), model->get_aabbmax())) {
                std::cout << "Collision with: " << model->name() << "\n";
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
    std::vector<GLenum> shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto blinnphong = std::make_shared<Shader>(shader_paths, shader_types, "blinn-phong");

    shader_paths = {"assets/shaders/depth_2d.vert",
                    "assets/shaders/depth_2d.frag"};
    shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    auto depth_2d = std::make_shared<Shader>(shader_paths, shader_types, "depth_2d");

      #ifdef DEBUG_DEPTH
      shader_paths = {"assets/shaders/depth_debug.vert", "assets/shaders/depth_debug.frag"};
      shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
      Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
      #endif

      shader_paths = {"assets/shaders/depth_cube.vert", "assets/shaders/depth_cube.geom",
      "assets/shaders/depth_cube.frag"}; shader_types = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER,
      GL_FRAGMENT_SHADER}; auto depth_cube = std::make_shared<Shader>(shader_paths, shader_types, "depth_cube");

    add_shader(blinnphong);
    add_shader(depth_2d);
    add_shader(depth_cube);
}

void Game::SceneManager::render() {

    auto flashlight = findLight("flashlight");
    if (!flashlight) {
        std::cout << "Light with name: " << "flashlight" << " was not found\n";
        assert(false);
    }
    float right_offset = 0.4f;
    // glm::vec3 offset = right_offset * camera.get_right() + forward_offset *
    // camera.get_direction();
    glm::vec3 offset = right_offset * camera.get_right();
    flashlight->set_position(camera.get_position() + offset);
    flashlight->set_direction(camera.get_direction());

    render_depth_pass();
    glm::mat4 view = camera.get_view_matrix();
    glm::mat4 proj = camera.get_projection_matrix();
    render(view, proj);
}

void Game::SceneManager::render(const glm::mat4& view, const glm::mat4& projection) {

    // Optional: reset viewport to screen size
    GLCall(glViewport(0, 0, screen_width, screen_height));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

     auto shader = get_shader_by_name("blinn-phong");

    shader->use();
    shader->set_int("numLights", (GLint)gameState.lights.size());

    for (size_t i = 0; i < gameState.lights.size(); ++i) {
        const Light* light = gameState.lights[i];
        std::string  base  = "lights[" + std::to_string(i) + "].";
        light->draw_lighting(shader, base, i);
        light->bind_shadow_map(shader, base, i);
    }

    for (auto const& model : gameState.models) {
        if (model->isActive()) {
            model->update_world_transform(glm::mat4(1.0f));
            if (model->is_instanced()) {
                model->draw_instanced(view, projection, shader);
            } else {
                model->draw(view, projection, shader);
            }
        }
    }

    glUseProgram(0);
}

Game::SceneManager::SceneManager(int width, int height, Camera::CameraObj camera,
                                 GameState gameState)
    : screen_width(width), screen_height(height), camera(camera), gameState(gameState) {}

Game::SceneManager::~SceneManager() {
    gameState.models.clear();
    shaders.clear();
    gameState.lights.clear();
}
