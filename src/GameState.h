
#pragma once
#include <Light.h>
#include <Model.h>
#include <memory>
#include <string>
#include <vector>


namespace Game {

class GameState {
  public:
    //should go to file
    inline GameState()
        : distance_from_closest_model(std::numeric_limits<float>::max()), closest_model("") {}

    float       distance_from_closest_model;
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

    unsigned int pages_collected        = 0;
    unsigned int pages_collected_to_win = 6;
    inline void         clear_models() {
        models.clear();        // the unique_ptrs—and hence the Model objects—are destroyed
        model_names.clear();   // clear the name list
        model_indices.clear(); // clear the lookup map
    }

    inline void clear_lights() {
        lights.clear();
        light_names.clear();
        light_indices.clear();
    }

  private:
    std::vector<std::unique_ptr<Models::Model>> models;
    std::vector<std::string>                    model_names;
    std::unordered_map<std::string, size_t>     model_indices;

    std::vector<std::unique_ptr<Light>>     lights;
    std::vector<std::string>                light_names;
    std::unordered_map<std::string, size_t> light_indices;
};

}; // namespace Game
