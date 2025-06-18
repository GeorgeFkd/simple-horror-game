// Room.h
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <utility>
#include <glm/glm.hpp>
#include "Model.h"

class Group {
public:
    using Rotation = std::pair<float, glm::vec3>; // angle in radians, axis

    Group(const std::string& room_name,
         const glm::vec3&   room_position);

    //All model names get prefixed by room_name + "-"
    Group& model(
        const std::string&       file,
        const std::string&       model_name,
        const glm::vec3&         position,
        std::optional<glm::vec3> scale       = std::nullopt,
        std::optional<Rotation>  rotation    = std::nullopt,
        bool                     interactive = false
    );
    //model id: <room_name> + "-" + <wall_model_name> + "-" + <wall_number>
    Group& walls(Models::Model& wall_model, float room_size);

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

