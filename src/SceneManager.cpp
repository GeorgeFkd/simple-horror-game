#include "SceneManager.h"
#include <iostream>

Shader* Game::SceneManager::get_shader_by_name(const std::string& shader_name) {
    for (auto shader : shaders) {
        if (shader->get_shader_name() == shader_name) {
            return shader;
        }
    }

    return nullptr;
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
    auto model_pos =
        std::find_if(gameState.models.begin(), gameState.models.end(), [name](auto* m) { return m->name() == name; });
    return *model_pos;
}

Light* Game::SceneManager::findLight(std::string_view name) {
    auto light_pos =
        std::find_if(gameState.lights.begin(), gameState.lights.end(), [name](auto* l) { return l->name() == name; });
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
    auto* depth2D   = get_shader_by_name("depth_2d");
    auto* depthCube = get_shader_by_name("depth_cube");

    for (auto* light : gameState.lights) {
        if (light->get_type() == LightType::POINT) {
            light->draw_depth_pass(depthCube, gameState.models);
        } else {
            light->draw_depth_pass(depth2D, gameState.models);
        }
    }
}

void Game::SceneManager::render(const glm::mat4& view, const glm::mat4& projection) {

    // Optional: reset viewport to screen size
    GLCall(glViewport(0, 0, screen_width, screen_height));
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    Shader* shader = get_shader_by_name("blinn-phong");
    if (!shader) {
        throw std::runtime_error("Could not find shader blinn-phong\n");
    }

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

Game::SceneManager::SceneManager(int width, int height)
    : screen_width(width), screen_height(height) {}

Game::SceneManager::~SceneManager() {
    gameState.models.clear();
    shaders.clear();
    gameState.lights.clear();
}
