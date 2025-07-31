#include <memory>
#include "GameState.h"
namespace Game {
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

} // namespace Game
