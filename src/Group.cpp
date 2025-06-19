#include "Group.h"
#include "ext/matrix_transform.hpp"
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
    //opposite side of the door,left side as you look at it
    auto wall_1_transform = glm::translate(id, position + glm::vec3(room_size, 0.0f,  4.6f));
    wall_1_transform = glm::scale(wall_1_transform,glm::vec3(1.0f,1.005f,1.1f));
    wall_model.add_instance_transform(
        wall_1_transform,
        prefix + "1"
    );

    //same side as the door, on its right
    auto wall_2_transform = glm::translate(id, position + glm::vec3(0.0f,       0.0f,  5.0f));
    wall_2_transform = glm::scale(wall_2_transform,glm::vec3(1.0f,1.005f,1.035f));
    wall_model.add_instance_transform(
        wall_2_transform,
        prefix + "2"
    );

    //same side as the door,on its left
    auto wall_3_transform = glm::translate(id, position + glm::vec3(0.0f,       0.0f, -6.65f));
    wall_3_transform = glm::scale(wall_3_transform,glm::vec3(1.0f,1.005f,1.05f));
    wall_model.add_instance_transform(
        wall_3_transform,
        prefix + "3"
    );
    
    //opposite side of the door, right side as you look at it
    auto wall_4_transform = glm::translate(id, position + glm::vec3(room_size, 0.0f, -6.25f));
    wall_4_transform = glm::scale(wall_4_transform,glm::vec3(1.0f,1.005f,1.1f));
    wall_model.add_instance_transform(
        wall_4_transform,
        prefix + "4"
    );
    
    //5,6 are the sides orthogonal to the door side
    auto wall_5_transform = glm::rotate(
            glm::translate(id, position + glm::vec3(5.5f, 0.0f,  room_size)),
            glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f)
        );
    wall_5_transform = glm::scale(wall_5_transform,glm::vec3(1.0f,1.005f,1.0f));
    wall_model.add_instance_transform(
        wall_5_transform,
        prefix + "5"
    );
    auto wall_6_transform = glm::rotate(
            glm::translate(id, position + glm::vec3(5.5f, 0.0f, -12.0f)),
            glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f)
        );
    wall_model.add_instance_transform(
        wall_6_transform,
        prefix + "6"
    );
    glm::mat4 roof_transform = glm::translate(id, position + glm::vec3(0.0f, 3.75f, -0.6f));
    roof_transform = glm::rotate(roof_transform, glm::radians(-90.0f), glm::vec3(0.0f,0.0f,1.0f));
    roof_transform = glm::scale(roof_transform, glm::vec3(1.0f,2.95f,2.21f));
    wall_model.add_instance_transform(roof_transform, prefix + "7");
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

