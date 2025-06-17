// Room.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <utility>
#include <glm/glm.hpp>
#include "Model.h"

class Room {
public:
    using Rotation = std::pair<float, glm::vec3>; // angle in radians, axis

    Room(const std::string& room_name,
         const glm::vec3&   room_position);

    // Add a model entry; optionally scaled, rotated, and interactive
    Room& model(
        const std::string&       file,
        const std::string&       model_name,
        const glm::vec3&         position,
        std::optional<glm::vec3> scale       = std::nullopt,
        std::optional<Rotation>  rotation    = std::nullopt,
        bool                     interactive = false
    );
    Room& walls(Models::Model& wall_model, const std::string& base_name, float room_size);

    // Build and return all models with transforms applied
    std::vector<std::unique_ptr<Models::Model>> models() const;

    inline glm::vec3 room_position() {
        return position;
    }

private:
    struct Entry {
        std::string file;
        std::string model_name;
        glm::vec3   position;
        std::optional<glm::vec3> scale;
        std::optional<Rotation>  rotation;
        bool        interactive;
    };

    std::string        name;
    glm::vec3          position;
    std::vector<Entry> entries;
};

