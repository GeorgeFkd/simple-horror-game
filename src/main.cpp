// main.cpp
#include "Camera.h"
#include "Group.h"
#include "Light.h"
#include "SceneManager.h"
#include "fwd.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

enum class SurfaceType { Floor, Ceiling, WallFront, WallBack, WallLeft, WallRight };
struct Models::Model repeating_tile(SurfaceType surface, float offset, const Material& material) {
    // might be able to constexpr this
    constexpr int   TILE_WIDTH  = 50;
    constexpr int   TILE_HEIGHT = 50;
    constexpr float half_width  = TILE_WIDTH / 2.0f;
    constexpr float half_height = TILE_HEIGHT / 2.0f;

    std::vector<glm::vec3> verts;
    std::vector<glm::vec2> texcoords;
    glm::vec3              normal;
    std::string            label;

    switch (surface) {
    case SurfaceType::Floor:
        verts  = {{-half_width, offset, -half_height},
                  {-half_width, offset, half_height},
                  {half_width, offset, half_height},
                  {half_width, offset, -half_height}};
        label  = "floor";
        normal = glm::vec3(0, 1, 0);
        break;

    case SurfaceType::Ceiling:
        verts  = {{-half_width, offset, -half_height},
                  {-half_width, offset, half_height},
                  {half_width, offset, half_height},
                  {half_width, offset, -half_height}};
        label  = "ceiling";
        normal = glm::vec3(0, -1, 0);
        break;

    case SurfaceType::WallFront:
        verts  = {{-half_width, -half_height, offset},
                  {-half_width, half_height, offset},
                  {half_width, half_height, offset},
                  {half_width, -half_height, offset}};
        label  = "wallfront";
        normal = glm::vec3(0, 0, 1);
        break;

    case SurfaceType::WallBack:
        verts  = {{half_width, -half_height, offset},
                  {-half_width, -half_height, offset},
                  {-half_width, half_height, offset},
                  {half_width, half_height, offset}};
        label  = "wallback";
        normal = glm::vec3(0, 0, -1);
        break;

    case SurfaceType::WallLeft:
        verts  = {{offset, -half_height, half_width},
                  {offset, half_height, half_width},
                  {offset, half_height, -half_width},
                  {offset, -half_height, -half_width}};
        label  = "wallleft";
        normal = glm::vec3(1, 0, 0);
        break;

    case SurfaceType::WallRight:
        verts  = {{offset, -half_height, -half_width},
                  {offset, half_height, -half_width},
                  {offset, half_height, half_width},
                  {offset, -half_height, half_width}};
        label  = "wallright";
        normal = glm::vec3(-1, 0, 0);
        break;
    }

    texcoords = {{0, 0}, {0, 1.0f}, {1.0f, 1.0f}, {1.0f, 0}};

    std::vector<glm::vec3> normals(4, normal);
    std::vector<GLuint>    indices = {0, 1, 2, 0, 2, 3};

    return Models::Model(verts, normals, texcoords, indices, std::move(label), material);
}

struct State {
    std::string_view closestModelLabel;
    float            closestModelDistance;
};

Models::Model createFloor(float roomSize) {

    float                  y           = 0.0f;
    std::vector<glm::vec3> floor_verts = {{-roomSize, y, -roomSize},
                                          {-roomSize, y, roomSize},
                                          {roomSize, y, roomSize},
                                          {roomSize, y, -roomSize}};
    std::vector<glm::vec3> floor_normals(4, glm::vec3(0, 1, 0));
    std::vector<glm::vec2> floor_uvs     = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    std::vector<GLuint>    floor_indices = {0, 1, 2, 0, 2, 3};

    Material floor_material;
    floor_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f); // dark ambient
    floor_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f); // brown diffuse
    floor_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f); // small specular
    floor_material.Ns    = 16.0f;                          // shininess
    floor_material.d     = 1.0f;                           // opacity
    floor_material.illum = 2;                              // standard Phong

    return Models::Model(floor_verts, floor_normals, floor_uvs, floor_indices, std::move("Floor"),
                         floor_material);
}

Models::Model createCeiling(float roomSize, float height) {
    std::vector<glm::vec3> floor_verts = {{-roomSize, height, -roomSize},
                                          {-roomSize, height, roomSize},
                                          {roomSize, height, roomSize},
                                          {roomSize, height, -roomSize}};
    std::vector<glm::vec3> floor_normals(4, glm::vec3(0, -1, 0));
    std::vector<glm::vec2> floor_uvs = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    // the indices are changed to agree with the normals 0,-1,0 as otherwise it is discarded
    std::vector<GLuint> floor_indices = {0, 2, 1, 0, 3, 2};

    Material floor_material;
    floor_material.Ka           = glm::vec3(0.15f, 0.07f, 0.02f); // dark ambient
    floor_material.Kd           = glm::vec3(0.59f, 0.29f, 0.00f); // brown diffuse
    floor_material.Ks           = glm::vec3(0.05f, 0.04f, 0.03f); // small specular
    floor_material.Ns           = 16.0f;                          // shininess
    floor_material.d            = 1.0f;                           // opacity
    floor_material.illum        = 2;                              // standard Phong
    floor_material.use_bump_map = false;

    return Models::Model(floor_verts, floor_normals, floor_uvs, floor_indices, std::move("Ceiling"),
                         floor_material);
}


void createRoom2(Game::GameState& game_state) {}

int main() {

    Camera::CameraObj  camera(1280, 720, glm::vec3(0.0f, 10.0f, 3.5f));
    Game::SceneManager scene_manager(1280, 720, camera);

    scene_manager.initialise_opengl_sdl();
    scene_manager.initialise_shaders();

    Game::GameState game_state;

    constexpr float ROOM_HEIGHT = 30.0f;
    constexpr float ROOM_WIDTH  = 60.0f;
    constexpr float ROOM_DEPTH  = ROOM_WIDTH;

    auto floor_model   = createFloor(ROOM_WIDTH);
    auto ceiling_model = createCeiling(ROOM_WIDTH, ROOM_HEIGHT);
    game_state.add_model(std::move(floor_model), floor_model.name());
    game_state.add_model(std::move(ceiling_model), ceiling_model.name());

    auto                   scroll        = Models::Model("assets/models/scroll.obj", "page");
    constexpr unsigned int extra_scrolls = 4;
    scroll.init_instancing(6 + 4);
    glm::vec3 scroll_positions[6];
    scroll_positions[0] = {-29.0f, 0.0f, -29.0f};
    scroll_positions[1] = {-15.0f, 0.f, 20.0f};
    scroll_positions[2] = {-5.0f, 0.0f, -20.0f};
    scroll_positions[3] = {5.0f, 0.0f, 20.0f};
    scroll_positions[4] = {29.0f, 0.0f, -29.0f};
    scroll_positions[5] = {29.0f, 0.0f, 29.0f};

    scroll.set_interactivity(true);
    for (int i = 0; i < 6; i++)
        scroll.add_instance_transform(glm::translate(glm::mat4(1.0f), scroll_positions[i]),
                                      "-" + std::to_string(i));

    auto wall =
        Models::Model("assets/models/SimpleOldTownAssets/OldHouseBrownWallLarge.obj", "wall");
    constexpr int   grid_rows                               = 10;
    constexpr int   grid_columns                            = 7;
    constexpr float wall_y                                  = 0.0f;
    const float     spacing_x                               = ROOM_WIDTH / (grid_columns - 1);
    const float     spacing_z                               = ROOM_DEPTH / (grid_rows - 1);
    bool            horizontal[grid_rows + 1][grid_columns] = {};
    bool            vertical[grid_rows][grid_columns + 1]   = {};

    // === Define internal walls ===
    horizontal[1][1] = true;
    horizontal[2][1] = true;
    horizontal[3][1] = true;
    horizontal[4][1] = true;
    horizontal[5][1] = true;
    horizontal[6][1] = true;
    horizontal[7][1] = true;
    horizontal[8][1] = true;

    horizontal[2][2] = true;
    horizontal[3][2] = true;
    horizontal[4][2] = true;
    horizontal[5][2] = true;
    horizontal[6][2] = true;
    horizontal[7][2] = true;
    horizontal[8][2] = true;

    horizontal[2][3] = true;
    horizontal[3][3] = true;
    horizontal[4][3] = true;
    horizontal[5][3] = true;
    horizontal[6][3] = true;
    horizontal[7][3] = true;
    horizontal[8][3] = true;

    horizontal[2][4] = true;
    horizontal[3][4] = true;
    horizontal[4][4] = true;
    horizontal[5][4] = true;
    horizontal[6][4] = true;
    horizontal[7][4] = true;
    horizontal[8][4] = true;

    horizontal[2][5] = true;
    horizontal[3][5] = true;
    horizontal[4][5] = true;
    horizontal[5][5] = true;
    horizontal[6][5] = true;
    horizontal[7][5] = true;
    horizontal[8][5] = true;
    horizontal[9][5] = true;

    vertical[8][2] = true;
    vertical[1][3] = true;
    vertical[8][4] = true;

    unsigned int extraWalls = 10;
    wall.init_instancing(grid_rows * grid_columns * 2 + extraWalls);

    float half_width  = (grid_columns - 1) * spacing_x / 2.0f;
    float half_height = (grid_rows - 1) * spacing_z / 2.0f;

    // place horizontal walls (Z-aligned)
    for (int row = 1; row < grid_rows; ++row) {
        for (int col = 0; col < grid_columns; ++col) {
            if (horizontal[row][col]) {
                float x = col * spacing_x - half_width;
                float z = (row - 0.5f) * spacing_z - half_height;

                glm::vec3 pos = glm::vec3(x, wall_y, z);
                glm::mat4 tf  = glm::translate(glm::mat4(1.0f), pos);
                wall.add_instance_transform(tf, "-h-" + std::to_string(row) + "-" +
                                                    std::to_string(col));
            }
        }
    }

    // place vertical walls (X-aligned, rotated)
    for (int row = 0; row < grid_rows; ++row) {
        for (int col = 1; col < grid_columns; ++col) {
            if (vertical[row][col]) {
                float x = (col - 0.5f) * spacing_x - half_width;
                float z = row * spacing_z - half_height;

                glm::vec3 pos = glm::vec3(x, wall_y, z);
                glm::mat4 tf  = glm::translate(glm::mat4(1.0f), pos);
                tf            = glm::rotate(tf, glm::radians(90.0f), glm::vec3(0, 1, 0));

                wall.add_instance_transform(tf, "-v-" + std::to_string(row) + "-" +
                                                    std::to_string(col));
            }
        }
    };

    auto right_light = Models::Model("assets/models/light_sphere.obj", "sphere");
    auto overhead_point_light_model =
        Models::Model("assets/models/light_sphere.obj", "overhead_point_light");
    auto right_spot_light_model =
        Models::Model("assets/models/light_sphere.obj", "right_spot_light");
    Light flashlight(
        LightType::SPOT,                 // 1
        glm::vec3(0.0f),                 // 2: position
        glm::vec3(0.0f, 0.0f, -1.0f),    // 3: direction
        glm::vec3(0.1f),                 // 4: ambient
        glm::vec3(1.0f),                 // 5: diffuse
        glm::vec3(1.0f),                 // 6: specular
        glm::cos(glm::radians(5.0f)),    // 7: cutoff (inner cone)
        glm::cos(glm::radians(15.0f)),   // 8: outer_cutoff
        1280,                            // 9: shadow_width
        720,                             // 10: shadow_height
        0.1f,                            // 11: near_plane
        50.0f,                          // 12: far_plane
        10.0f,                           // 13: ortho_size
        1.0f,                            // 14: attenuation_constant
        0.35f,                           // 15: attenuation_linear
        0.05f,                           // 16: attenuation_quadratic
        1.0f,                            // 17: attenuation_power
        10.0f,                            // 18: light_power
        true,                            // 19: is_on
        "flashlight",                    // 20: label
        glm::vec3(1.0f)                  // 21: color
    );

    Light spotlight(
        LightType::SPOT,                 // 1: light type
        glm::vec3(10.0f, 10.0f, 0.0f),    // 2: position (to the right)
        glm::vec3(0.0f, -1.0f, 0.0f),    // 3: direction (pointing down)
        glm::vec3(0.1f),                 // 4: ambient
        glm::vec3(1.0f),                 // 5: diffuse
        glm::vec3(1.0f),                 // 6: specular
        glm::cos(glm::radians(10.0f)),   // 7: inner cone (cutoff)
        glm::cos(glm::radians(30.0f)),   // 8: outer cone
        2048,                            // 9: shadow_width
        2048,                            // 10: shadow_height
        1.0f,                            // 11: near_plane
        10.0f,                           // 12: far_plane
        10.0f,                           // 13: ortho_size
        1.0f,                            // 14: attenuation_constant
        0.35f,                           // 15: attenuation_linear
        0.01f,                           // 16: attenuation_quadratic
        1.0f,                            // 17: attenuation_power
        3.0f,                            // 18: light_power
        true,                            // 19: is_on
        "spotlight",               // 20: label
        glm::vec3(1.0f)                  // 21: color
    );

    Light pointlight(
        LightType::POINT,                   // 1: light type
        glm::vec3(0.0f, 5.0f, 0.0f),        // 2: position (above the object)
        glm::vec3(0.0f, -1.0f, 0.0f),       // 3: direction (pointing straight down)
        glm::vec3(0.1f),                    // 4: ambient color
        glm::vec3(1.0f),                    // 5: diffuse color
        glm::vec3(1.0f),                    // 6: specular color
        glm::cos(glm::radians(10.0f)),      // 7: cutoff (inner cone) — unused for POINT
        glm::cos(glm::radians(30.0f)),      // 8: outer_cutoff — unused for POINT
        2048,                               // 9: shadow_width
        2048,                               // 10: shadow_height
        1.0f,                               // 11: near_plane
        25.0f,                              // 12: far_plane
        10.0f,                              // 13: ortho_size       — unused for POINT
        1.0f,                               // 14: attenuation_constant
        0.35f,                              // 15: attenuation_linear
        0.01f,                              // 16: attenuation_quadratic
        1.0f,                               // 17: attenuation_power
        3.0f,                               // 18: light_power
        true,                               // 19: is_on
        "overhead_pointlight",              // 20: label
        glm::vec3(0.4f)                     // 21: color tint
    );

    game_state.add_light(std::move(flashlight), "flashlight");

    std::vector<std::string> random_objects = {"assets/models/SimpleOldTownAssets/TableSmall1.obj",
                                               "assets/models/SimpleOldTownAssets/PotNtural.obj",
                                               "assets/models/SimpleOldTownAssets/workbench01.obj",
                                               "assets/models/SimpleOldTownAssets/watering-can.obj",
                                               "assets/models/axe.obj"};

    auto scatter_fn = [&](float radius, const glm::vec3& center,
                          const std::vector<std::string>& objects) {
        std::random_device                    rd;
        std::mt19937                          rng(rd());
        std::uniform_real_distribution<float> dist(-radius, radius);
        // Build the Group
        Group group("scatter-objects", center);

        // Shuffle paths
        auto shuffled = objects;
        std::shuffle(shuffled.begin(), shuffled.end(), rng);

        for (int i = 0; i < (int)shuffled.size(); ++i) {
            // random translation
            glm::vec3 offset{dist(rng), 0.0f, dist(rng)};

            group.model(shuffled[i],
                        "scatter-" + std::to_string(i),
                        offset
            );
        }

        return group;
    };

    // auto scattered_group_1 = scatter_fn(4.0f, {30.0f, 0.0f, -15.0f}, random_objects);
    // for (auto& m : scattered_group_1.models()) {
    //     game_state.add_model(std::move(m), m->name());
    // }
    //
    // auto scattered_group_2 = scatter_fn(4.0f, {30.0f, 0.0f, -45.0f}, random_objects);
    // for (auto& m : scattered_group_2.models()) {
    //     game_state.add_model(std::move(m), m->name());
    // }
    //
    // std::vector<std::string> random_objects_2 = {
    //     "assets/models/old_office.obj", "assets/models/surgery_tools.obj",
    //     "assets/models/SimpleOldTownAssets/workbench01.obj"};
    //
    // auto scattered_group_3 = scatter_fn(6.0f, {-10.0f, 0.0f, -45.0f}, random_objects_2);
    // for (auto& m : scattered_group_3.models()) {
    //     game_state.add_model(std::move(m), m->name());
    // }
    //
    //
    // auto scattered_group_4 = scatter_fn(4.0f, {-30.0f, 0.0f, 10.0f}, random_objects);
    // for (auto& m : scattered_group_4.models()) {
    //     game_state.add_model(std::move(m), m->name());
    // }
    glm::vec3 overhead_point = glm::vec3(15.0f, -4.0f, -20.0f);
    pointlight.set_position(overhead_point);
    overhead_point_light_model.set_local_transform(
        glm::translate(glm::mat4(1.0f), pointlight.get_position()));

    // ROOMS
    float     room_size   = 10.0f;
    auto      door_scale  = glm::vec3(1.3f, 1.45f, 1.3f);
    glm::vec3 room_offset = glm::vec3(ROOM_WIDTH - room_size, 0.0f, ROOM_DEPTH - room_size);
    Group     room1("room-1", room_offset);
    room1
        .model("assets/models/SimpleOldTownAssets/OldHouseDoorWoodDarkRed.obj", "door",
               glm::vec3(0.0f), door_scale, {}, true)
        .walls(wall, room_size)
        .model("assets/models/SimpleOldTownAssets/ChairCafeBrown01.obj", "chair",
               glm::vec3(5.0f, 0.0f, 5.0f))
        .model("assets/models/SimpleOldTownAssets/TableSmall1.obj", "small_table",
               glm::vec3(5.0f, 0.0f, -4.0f))
        .model("assets/models/SimpleOldTownAssets/leaves.obj", "leaves",
               glm::vec3(3.0f, 0.0f, -6.0f))
        .model("assets/models/SimpleOldTownAssets/Flokati.obj", "flokati",
               glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(5.5f, 1.0f, 5.5f))
        .model("assets/models/SimpleOldTownAssets/Bed02.obj", "bed", glm::vec3(5.0f, 0.0f, 0.0f),
               glm::vec3(1.2f, 1.2f, 1.2f))
        .model("assets/models/scroll.obj", "page-7", glm::vec3(5.0f, 0.0f, 8.0f));

    // now pick a local position *inside* the room, e.g. 2 units in front of the door and 3 up:
    glm::vec3 right_light_local = glm::vec3(4.0f, 1.0f, -10.0f);
    // convert to world‐space by adding the room’s offset:
    glm::vec3 right_light_world = room_offset + right_light_local;
    spotlight.set_direction(glm::vec3(0.0f, 0.0f, 1.0f));
    // move your spotlight there:
    spotlight.set_position(right_light_world);
    // if you have a model to visualize the light, move it too:
    right_spot_light_model.set_local_transform(
        glm::translate(glm::mat4(1.0f), right_light_world)
    );
    game_state.add_light(std::move(spotlight), "spotlight");
    auto room1_models = room1.models();
    for (auto& m : room1_models) {
        game_state.add_model(std::move(m), m->name());
    }

    glm::vec3 room_offset2 = glm::vec3(-ROOM_WIDTH + room_size, 0.0f, -ROOM_DEPTH + room_size * 2);
    Group     room2("room-2", room_offset2);
    room2
        .model("assets/models/SimpleOldTownAssets/OldHouseDoorWoodDarkRed.obj", "door",
               glm::vec3(0.0f), door_scale, std::nullopt, true)
        .walls(wall, room_size)

        .model("assets/models/SimpleOldTownAssets/ChairCafeBrown01.obj", "chair",
               glm::vec3(5.0f, 0.0f, 5.0f))
        .model("assets/models/SimpleOldTownAssets/TableSmall1.obj", "small_table",
               glm::vec3(5.0f, 0.0f, -4.0f))
        .model("assets/models/SimpleOldTownAssets/leaves.obj", "leaves",
               glm::vec3(3.0f, 0.0f, -6.0f), glm::vec3(1.2f))
        .model("assets/models/SimpleOldTownAssets/Flokati.obj", "flokati",
               glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(5.5f, 1.0f, 5.5f))

        .model("assets/models/SimpleOldTownAssets/Bed02.obj", "bed", glm::vec3(5.0f, 0.0f, 0.0f),
               glm::vec3(1.2f, 1.2f, 1.2f));

    // pick a local position inside the 10×10 room, say 3 units in on X/Z and 5 up:
    glm::vec3 point_local2 = glm::vec3(5.0f, 2.0f, 3.0f);
    // convert to world space
    glm::vec3 point_world2 = room_offset2 + point_local2;

    // move point‐light there and have it shine down
    pointlight.set_position(point_world2);
    pointlight.set_direction(glm::vec3(0.0f, -1.0f, 0.0f));

    overhead_point_light_model.set_local_transform(
        glm::translate(glm::mat4(1.0f), point_world2)
    );

    game_state.add_light(std::move(pointlight), "pointlight");
    //game_state.add_model(std::move(overhead_point_light_model), "overhead_pointlight_model");


    for (auto& m : room2.models()) {
        game_state.add_model(std::move(m), m->name());
    }

    glm::vec3 room_offset3 = glm::vec3(ROOM_WIDTH - room_size, 0.0f, -ROOM_DEPTH + room_size * 2);
    Group     room3("room-3", room_offset3);
    room3
        .model("assets/models/SimpleOldTownAssets/OldHouseDoorWoodDarkRed.obj", "door",
               glm::vec3(0.0f), door_scale, std::nullopt, true)
        .walls(wall, room_size)
        .model("assets/models/SimpleOldTownAssets/ChairCafeBrown01.obj", "chair",
               glm::vec3(5.0f, 0.0f, 5.0f))
        .model("assets/models/SimpleOldTownAssets/TableSmall1.obj", "small_table",
               glm::vec3(5.0f, 0.0f, -4.0f))
        .model("assets/models/SimpleOldTownAssets/leaves.obj", "leaves",
               glm::vec3(3.0f, 0.0f, -6.0f), glm::vec3(1.2f))
        .model("assets/models/SimpleOldTownAssets/Flokati.obj", "flokati",
               glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(5.5f, 1.0f, 5.5f))
        .model("assets/models/SimpleOldTownAssets/Bed02.obj", "bed", glm::vec3(5.0f, 0.0f, 0.0f),
               glm::vec3(1.2f, 1.2f, 1.2f));

    for (auto& m : room3.models()) {
        game_state.add_model(std::move(m), m->name());
    }

    glm::vec3 page3_pos = room_offset3 + glm::vec3(5.0f, 0.0f, 8.0f);
    scroll.add_instance_transform(glm::translate(scroll.get_local_transform(), page3_pos),
                                  "room-3-page-9");

    glm::vec3 room_offset4 = glm::vec3(-ROOM_WIDTH + room_size, 0.0f, ROOM_DEPTH - room_size);
    Group     room4("room-4", room_offset4);
    room4
        .model("assets/models/SimpleOldTownAssets/OldHouseDoorWoodDarkRed.obj", "door",
               glm::vec3(0.0f), door_scale, std::nullopt,
               true // interactive
               )
        .model("assets/models/SimpleOldTownAssets/ChairCafeBrown01.obj", "chair",
               glm::vec3(5.0f, 0.0f, 5.0f))
        .model("assets/models/SimpleOldTownAssets/TableSmall1.obj", "small_table",
               glm::vec3(5.0f, 0.0f, -4.0f))
        .model("assets/models/SimpleOldTownAssets/leaves.obj", "leaves",
               glm::vec3(3.0f, 0.0f, -6.0f))
        .model("assets/models/SimpleOldTownAssets/Flokati.obj", "flokati",
               glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(5.5f, 1.0f, 5.5f))
        .model("assets/models/SimpleOldTownAssets/Bed02.obj", "bed", glm::vec3(5.0f, 0.0f, 0.0f),
               glm::vec3(1.2f, 1.2f, 1.2f))
        .walls(wall, room_size);

    auto room4_models = room4.models();
    for (auto& m : room4_models) {
        //std::cout << "Model: " << m->name() << std::endl;
        game_state.add_model(std::move(m), m->name());
    }

    Group dining_room("dining-room", glm::vec3(ROOM_DEPTH - ROOM_DEPTH / 4, 0.0f, 0.0f));
    dining_room.model("assets/models/SimpleOldTownAssets/OldHouseDoorWoodDarkRed.obj", "door",
                      glm::vec3(0.0f), door_scale, std::nullopt,
                      true // interactive
    );
    dining_room.model("assets/models/SimpleOldTownAssets/dining-place.obj", "diner",
                      glm::vec3(4.0f, 0.0f, 4.0f));
    dining_room.model("assets/models/SimpleOldTownAssets/Flokati.obj", "flokati",
                      glm::vec3(4.0f, 0.0f, 4.0f), glm::vec3(5.5f, 1.0f, 5.5f),
                      std::make_pair(90.0f, glm::vec3(0.0f, 1.0f, 0.0f)), false);
    dining_room.model("assets/models/SimpleOldTownAssets/leaves.obj", "leaves",
                      glm::vec3(7.0f, 0.0f, 2.0f));
    dining_room.walls(wall, room_size);

    for (auto& m : dining_room.models()) {
        game_state.add_model(std::move(m), m->name());
    }

    game_state.add_model(std::move(scroll), "page");
    game_state.add_model(std::move(wall), "wall");

    scene_manager.set_game_state(game_state);
    struct DoorState {
        bool      is_open     = false;
        bool      initialized = false;
        glm::mat4 closed_xf   = glm::mat4(1.0f);
        glm::mat4 open_xf     = glm::mat4(1.0f);
    };
    constexpr size_t                   NUM_DOORS     = 5;
    std::array<std::string, NUM_DOORS> doors_of_game = {
        "dining-room-door", "room-1-door", "room-2-door", "room-3-door", "room-4-door"};
    std::array<DoorState, NUM_DOORS> door_states;
    for (size_t i = 0; i < NUM_DOORS; ++i) {
        const auto& door_name = doors_of_game[i];
        DoorState&  state     = door_states[i];
        scene_manager.bind_handler_to_model(
            door_name, [&state, door_name](Game::SceneManager* scene_manager) mutable {
                auto door_to_toggle = scene_manager->get_game_state()->find_model(door_name);
                if (!door_to_toggle) {
                    throw std::runtime_error("Door Model not found: " + door_name);
                }

                if (!state.initialized) {
                    state.initialized = true;
                    state.closed_xf   = door_to_toggle->get_local_transform();
                    // TODO door does not open to the correct side, it needs a couple extra
                    // transforms
                    state.open_xf = glm::rotate(state.closed_xf, glm::radians(-90.0f),
                                                glm::vec3(0.0f, 1.0f, 0.0f));
                }
                if (state.is_open) {
                    door_to_toggle->set_local_transform(state.closed_xf);
                } else {
                    door_to_toggle->set_local_transform(state.open_xf);
                }
                state.is_open = !state.is_open;
                return true;
            });
    }

    for (size_t i = 0; i < 6; i++) {
        auto name = std::string("page-") + std::to_string(i);
        scene_manager.bind_handler_to_model(name, [i](Game::SceneManager* scene_manager) {
            auto m = scene_manager->get_game_state()->find_model("page");
            if (!m) {
                throw std::runtime_error("Model not found " + m->name());
            }
            scene_manager->remove_instanced_model_at(m->name(-1), "-" + std::to_string(i));
            scene_manager->get_game_state()->pages_collected += 1;

            if (m->get_active_instance_count() == 0) {
                m->disable();
            }
            return false;
        });
    }
    scene_manager.run_game_loop();

    return 0;
}
