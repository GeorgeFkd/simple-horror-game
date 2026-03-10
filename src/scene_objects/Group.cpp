#include "Group.h"
#include <glm/ext/matrix_transform.hpp>
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

Group& Group::walls(const Models::Model& wall_model,
                  float room_size)
{
    glm::mat4 id(1.0f);
    std::string prefix = name + "-" + "wall" + "-";
    //opposite side of the door,left side as you look at it
    auto wall_1_transform = glm::translate(id, position + glm::vec3(room_size, 0.0f,  4.6f));
    wall_1_transform = glm::scale(wall_1_transform,glm::vec3(1.0f,1.005f,1.1f));
    auto wall1 = std::make_unique<Models::Model>(wall_model,prefix + "1",wall_1_transform);

    //same side as the door, on its right
    auto wall_2_transform = glm::translate(id, position + glm::vec3(0.0f,       0.0f,  4.8f));
    wall_2_transform = glm::scale(wall_2_transform,glm::vec3(1.0f,1.005f,1.04f));
    auto wall2 = std::make_unique<Models::Model>(wall_model,prefix + "1",wall_2_transform);

    //same side as the door,on its left
    auto wall_3_transform = glm::translate(id, position + glm::vec3(0.0f,       0.0f, -6.8f));
    wall_3_transform = glm::scale(wall_3_transform,glm::vec3(1.0f,1.005f,1.0015f));
    auto wall3 = std::make_unique<Models::Model>(wall_model,prefix + "1",wall_3_transform);
    
    //opposite side of the door, right side as you look at it
    auto wall_4_transform = glm::translate(id, position + glm::vec3(room_size, 0.0f, -6.25f));
    wall_4_transform = glm::scale(wall_4_transform,glm::vec3(1.0f,1.005f,1.1f));
    auto wall4 = std::make_unique<Models::Model>(wall_model,prefix + "4",wall_4_transform);
    
    //5,6 are the sides orthogonal to the door side
    auto wall_5_transform = glm::rotate(
            glm::translate(id, position + glm::vec3(5.5f, 0.0f,  room_size)),
            glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f)
        );
    wall_5_transform = glm::scale(wall_5_transform,glm::vec3(1.0f,1.005f,1.0f));
    auto wall5 = std::make_unique<Models::Model>(wall_model,prefix + "5",wall_5_transform);
    auto wall_6_transform = glm::rotate(
            glm::translate(id, position + glm::vec3(5.5f, 0.0f, -12.0f)),
            glm::radians(90.0f), glm::vec3(0.0f,1.0f,0.0f)
        );
    auto wall6 = std::make_unique<Models::Model>(wall_model,prefix + "6",wall_6_transform);
    glm::mat4 roof_transform = glm::translate(id, position + glm::vec3(0.0f, 3.75f, -0.6f));
    roof_transform = glm::rotate(roof_transform, glm::radians(-90.0f), glm::vec3(0.0f,0.0f,1.0f));
    roof_transform = glm::scale(roof_transform, glm::vec3(1.0f,2.95f,2.21f));
    auto wall7 = std::make_unique<Models::Model>(wall_model,prefix + "7", roof_transform);
    
    added_models.reserve(7);
    added_models.push_back(std::move(wall1));
    added_models.push_back(std::move(wall2));
    added_models.push_back(std::move(wall3));
    added_models.push_back(std::move(wall4));
    added_models.push_back(std::move(wall5));
    added_models.push_back(std::move(wall6));
    added_models.push_back(std::move(wall7));


    return *this;
}


std::vector<std::unique_ptr<Models::Model>> Group::models() {
    added_models.reserve(entries.size());
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
        added_models.push_back(std::move(m));
    }

    return std::move(added_models);
}

