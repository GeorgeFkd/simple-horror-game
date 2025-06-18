#include "Group.h"
#include <glm/gtc/matrix_transform.hpp>

Group::Group(const std::string& room_name,
           const glm::vec3&   room_position)
    : name(room_name), position(room_position) {}

Group& Group::model(
    const std::string&       file,
    const std::string&       model_name,
    const glm::vec3&         position,
    std::optional<glm::vec3> scale,
    std::optional<Rotation>  rotation,
    bool                     interactive)
{
    entries.push_back({file, model_name, position, scale, rotation, interactive});
    return *this;
}

Group& Group::walls(Models::Model& wall_model,
                  float room_size)
{
    glm::mat4 id(1.0f);
    std::string prefix = name + "-" + wall_model.name() + "-";
    wall_model.add_instance_transform(
        glm::translate(id, position + glm::vec3(room_size, 0.0f,  5.0f)),
        prefix + "1"
    );
    wall_model.add_instance_transform(
        glm::translate(id, position + glm::vec3(0.0f,       0.0f,  4.8f)),
        prefix + "2"
    );
    wall_model.add_instance_transform(
        glm::translate(id, position + glm::vec3(0.0f,       0.0f, -6.75f)),
        prefix + "3"
    );
    wall_model.add_instance_transform(
        glm::translate(id, position + glm::vec3(room_size, 0.0f, -5.0f)),
        prefix + "4"
    );
    wall_model.add_instance_transform(
        glm::rotate(
            glm::translate(id, position + glm::vec3(5.5f, 0.0f,  room_size)),
            glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f)
        ),
        prefix + "5"
    );
    wall_model.add_instance_transform(
        glm::rotate(
            glm::translate(id, position + glm::vec3(5.5f, 0.0f, -12.0f)),
            glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f)
        ),
        prefix + "6"
    );
    // roof piece
    glm::mat4 roof = glm::translate(id, position + glm::vec3(0.0f, 3.75f, -0.6f));
    roof = glm::rotate(roof, glm::radians(-90.0f), glm::vec3(0.0f,0.0f,1.0f));
    roof = glm::scale(roof, glm::vec3(1.0f,2.8f,2.2f));
    wall_model.add_instance_transform(roof, prefix + "7");
    return *this;
}


std::vector<std::unique_ptr<Models::Model>> Group::models() const {
    std::vector<std::unique_ptr<Models::Model>> result;
    result.reserve(entries.size());

    for (auto& e : entries) {
        auto m = std::make_unique<Models::Model>(e.file, name + "-" + e.model_name);
        glm::mat4 xf = m->get_local_transform();
        xf = glm::translate(xf, position + e.position);
        if (e.scale) {
            xf = glm::scale(xf, *e.scale);
        }
        if (e.rotation) {
            xf = glm::rotate(xf, glm::radians(e.rotation->first), e.rotation->second);
        }
        m->set_local_transform(xf);
        if (e.interactive) {
            m->set_interactivity(true);
        }
        result.push_back(std::move(m));
    }

    return result;
}

