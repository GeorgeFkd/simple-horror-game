// main.cpp
#include "Camera.h"
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
    floor_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f); // dark ambient
    floor_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f); // brown diffuse
    floor_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f); // small specular
    floor_material.Ns    = 16.0f;                          // shininess
    floor_material.d     = 1.0f;                           // opacity
    floor_material.illum = 2;                              // standard Phong

    return Models::Model(floor_verts, floor_normals, floor_uvs, floor_indices, std::move("Ceiling"),
                         floor_material);
}

void createRoom2(Game::GameState& game_state) {

}

int main() {

    Camera::CameraObj  camera(1280, 720, glm::vec3(0.0f, 10.0f, 3.5f));
    Game::SceneManager scene_manager(1280, 720, camera);

    scene_manager.initialise_opengl_sdl();
    scene_manager.initialise_shaders();

    Game::GameState game_state;

    constexpr float ROOM_HEIGHT = 50.0f;
    constexpr float ROOM_WIDTH  = 60.0f;
    constexpr float ROOM_DEPTH  = ROOM_WIDTH;

    auto floor_model   = createFloor(ROOM_WIDTH);
    auto ceiling_model = createCeiling(ROOM_WIDTH, 30.0f);
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
                // tf = glm::scale(glm::mat4(1.0f),{1.0f,1.0f,1.35f}) * tf;
                std::cout << "[H] Placing wall between row " << (row - 1) << " and " << row
                          << " at col " << col << " -> Position: (" << x << ", " << wall_y << ", "
                          << z << ")\n";

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
                std::cout << "[V] Placing wall between col " << (col - 1) << " and " << col
                          << " at row " << row << " -> Position: (" << x << ", " << wall_y << ", "
                          << z << ")\n";

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
    Light flashlight(LightType::SPOT,
                     glm::vec3(0.0f),               // position
                     glm::vec3(0.0f, 0.0f, -1.0f),  // direction
                     glm::vec3(0.1f),               // ambient
                     glm::vec3(1.0f),               // diffuse
                     glm::vec3(1.0f),               // specular
                     glm::cos(glm::radians(10.0f)), // cutoff
                     glm::cos(glm::radians(20.0f)), // outer cutoff
                     1280, 720, 0.1f, 500.0f, 10.0f, 1.0f, 0.35f, 0.44f, 1.0f, 1.0f);
    flashlight.set_name("flashlight");
    // Light right_spotlight(LightType::SPOT, glm::vec3(5.0f, 1.5f, 0.0f), // position: to the right
    //                       glm::vec3(1.0f, 0.0f, -1.0f),                 // direction: pointing
    //                       left glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f),
    //                       glm::cos(glm::radians(10.0f)), // inner cone
    //                       glm::cos(glm::radians(30.0f)), // outer cone
    //                       2048, 2048, 1.0f, 10.0f, 10.0f, 1.0f, 0.35f, 0.44f, 1.0f, 1.0f);
    //
    // Light overhead_pointlight(LightType::POINT, glm::vec3(0.0f, 5.0f, 0.0f), // above the object
    //                           glm::vec3(0.0f, -1.0f, 0.0f), // pointing straight down
    //                           glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f),
    //                           glm::cos(glm::radians(10.0f)), // inner cone
    //                           glm::cos(glm::radians(30.0f)), // outer cone
    //                           2048, 2048, 0.1f, 10.0f, 10.0f, 1.0f, 0.35f, 0.44f, 1.0f, 1.0f);
    //
    // glm::vec3 overhead_spotlight = glm::vec3(15.0f, 5.0f, -20.0f);
    // overhead_pointlight.set_position(overhead_spotlight);
    // overhead_point_light_model.set_local_transform(
    //     glm::translate(glm::mat4(1.0f), overhead_pointlight.get_position()));
    //
    // glm::vec3 right_light_spot = glm::vec3(15.0f, 2.0f, -25.0f);
    // right_spotlight.set_position(right_light_spot);
    // right_spot_light_model.set_local_transform(
    //     glm::translate(glm::mat4(1.0f), right_spotlight.get_position()));

    // gameState.add_model(overhead_point_light_model);
    // gameState.add_model(right_spot_light_model);
    // scene_manager.add_light(overhead_point_light);
    game_state.add_light(std::move(flashlight), "flashlight");
    // gameState.add_light(right_spotlight);

    // Creating Rooms
    float room_size = 10.0f;

    // Compute the “origin” offset of the room within the larger map
    glm::vec3 room_offset = glm::vec3(ROOM_WIDTH - room_size, 0.0f, ROOM_DEPTH - room_size);

    auto door =
        Models::Model("assets/models/SimpleOldTownAssets/OldHouseDoorWoodDarkRed.obj", "door");
    glm::vec3 door_pos = room_offset;
    glm::mat4 door_tf  = glm::translate(door.get_local_transform(), door_pos);
    door_tf            = glm::scale(door_tf, glm::vec3(1.2f));
    door.set_local_transform(door_tf);
    door.set_interactivity(true);
    game_state.add_model(std::move(door), door.name());

    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset + glm::vec3(room_size, 0.0f, 5.0f)),
        "wall-room-1");
    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset + glm::vec3(0.0f, 0.0f, 4.8f)), "wall-room-2");
    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset + glm::vec3(0.0f, 0.0f, -6.75f)),
        "wall-room-3");
    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset + glm::vec3(room_size, 0.0f, -5.0f)),
        "wall-room-4");
    wall.add_instance_transform(
        glm::rotate(glm::translate(glm::mat4(1.0f), room_offset + glm::vec3(5.5f, 0.0f, room_size)),
                    glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        "wall-room-5");
    wall.add_instance_transform(
        glm::rotate(glm::translate(glm::mat4(1.0f), room_offset + glm::vec3(5.5f, 0.0f, -12.0f)),
                    glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        "wall-room-6");

    auto chair = Models::Model("assets/models/SimpleOldTownAssets/ChairCafeBrown01.obj", "chair");
    glm::vec3 chair_pos = room_offset + glm::vec3(5.0f, 0.0f, 5.0f);
    glm::mat4 chair_tf  = glm::translate(chair.get_local_transform(), chair_pos);
    chair.set_local_transform(chair_tf);
    chair.set_interactivity(false);
    game_state.add_model(std::move(chair), chair.name());

   
    auto small_table =
        Models::Model("assets/models/SimpleOldTownAssets/TableSmall1.obj", "small_table");
    glm::vec3 table_pos = room_offset + glm::vec3(5.0f, 0.0f, -4.0f);
    glm::mat4 table_tf  = glm::translate(small_table.get_local_transform(), table_pos);
    table_tf            = glm::scale(table_tf, glm::vec3(1.0f));
    small_table.set_local_transform(table_tf);
    small_table.set_interactivity(false);
    game_state.add_model(std::move(small_table), small_table.name());

    auto      leaves     = Models::Model("assets/models/SimpleOldTownAssets/leaves.obj", "leaves");
    glm::vec3 leaves_pos = room_offset + glm::vec3(3.0f, 0.0f, -6.0f);
    glm::mat4 leaves_tf  = glm::translate(leaves.get_local_transform(), leaves_pos);
    leaves_tf            = glm::scale(leaves_tf, glm::vec3(1.2f));
    leaves.set_local_transform(leaves_tf);
    leaves.set_interactivity(false);
    game_state.add_model(std::move(leaves), leaves.name());

    auto      flokati = Models::Model("assets/models/SimpleOldTownAssets/Flokati.obj", "flokati");
    glm::vec3 flokati_pos = room_offset + glm::vec3(5.0f, 0.0f, 0.0f);
    glm::mat4 flokati_tf  = glm::translate(flokati.get_local_transform(), flokati_pos);
    flokati_tf            = glm::scale(flokati_tf, glm::vec3(5.5f, 1.0f, 5.5f));
    flokati.set_local_transform(flokati_tf);
    flokati.set_interactivity(false);
    game_state.add_model(std::move(flokati), flokati.name());

    auto      bed     = Models::Model("assets/models/SimpleOldTownAssets/Bed02.obj", "bed");
    glm::vec3 bed_pos = room_offset + glm::vec3(5.0f, 0.0f, 0.0f);
    glm::mat4 bed_tf  = glm::translate(bed.get_local_transform(), bed_pos);
    bed_tf            = glm::scale(bed_tf, glm::vec3(1.2f, 1.2f, 1.2f));
    bed.set_local_transform(bed_tf);
    bed.set_interactivity(false);

    auto page_pos = room_offset + glm::vec3(5.0f,0.0f,8.0f);
    scroll.add_instance_transform(glm::translate(scroll.get_local_transform(),page_pos),"page-7");
    game_state.add_model(std::move(bed), bed.name());
    
    //room 2
    glm::vec3 room_offset2 = glm::vec3(-ROOM_WIDTH + room_size, 0.0f, -ROOM_DEPTH + room_size * 2);
    std::string prefix = "room-" + std::to_string(2) + "-";

    auto door2 =
        Models::Model("assets/models/SimpleOldTownAssets/OldHouseDoorWoodDarkRed.obj", prefix + "door");
    glm::vec3 door2_pos = room_offset2;
    glm::mat4 door2_tf  = glm::translate(door2.get_local_transform(), door2_pos);
    door2_tf            = glm::scale(door2_tf, glm::vec3(1.2f));
    door2.set_local_transform(door2_tf);
    door2.set_interactivity(true);
    game_state.add_model(std::move(door2), door2.name());

    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset2 + glm::vec3(room_size, 0.0f, 5.0f)),
        prefix + "wall-room-1");
    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset2 + glm::vec3(0.0f, 0.0f, 4.8f)), prefix + "wall-room-2");
    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset2 + glm::vec3(0.0f, 0.0f, -6.75f)),
        prefix + "wall-room-3");
    wall.add_instance_transform(
        glm::translate(glm::mat4(1.0f), room_offset2 + glm::vec3(room_size, 0.0f, -5.0f)),
        prefix + "wall-room-4");
    wall.add_instance_transform(
        glm::rotate(glm::translate(glm::mat4(1.0f), room_offset2 + glm::vec3(5.5f, 0.0f, room_size)),
                    glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        prefix + "wall-room-5");
    wall.add_instance_transform(
        glm::rotate(glm::translate(glm::mat4(1.0f), room_offset2 + glm::vec3(5.5f, 0.0f, -12.0f)),
                    glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        prefix + "wall-room-6");

    auto chair2 = Models::Model("assets/models/SimpleOldTownAssets/ChairCafeBrown01.obj", prefix + "chair");
    glm::vec3 chair2_pos = room_offset2 + glm::vec3(5.0f, 0.0f, 5.0f);
    glm::mat4 chair2_tf  = glm::translate(chair2.get_local_transform(), chair2_pos);
    chair2.set_local_transform(chair2_tf);
    chair2.set_interactivity(false);
    game_state.add_model(std::move(chair2), chair2.name());

    

   
    auto small_table2 =
        Models::Model("assets/models/SimpleOldTownAssets/TableSmall1.obj", prefix + "small_table");
    glm::vec3 table_pos2 = room_offset2 + glm::vec3(5.0f, 0.0f, -4.0f);
    glm::mat4 table_tf2  = glm::translate(small_table2.get_local_transform(), table_pos2);
    table_tf2            = glm::scale(table_tf2, glm::vec3(1.0f));
    small_table2.set_local_transform(table_tf2);
    small_table2.set_interactivity(false);
    game_state.add_model(std::move(small_table2), small_table2.name());

    auto      leaves2     = Models::Model("assets/models/SimpleOldTownAssets/leaves.obj", prefix + "leaves");
    glm::vec3 leaves_pos2 = room_offset2 + glm::vec3(3.0f, 0.0f, -6.0f);
    glm::mat4 leaves_tf2  = glm::translate(leaves2.get_local_transform(), leaves_pos2);
    leaves_tf2            = glm::scale(leaves_tf2, glm::vec3(1.2f));
    leaves2.set_local_transform(leaves_tf2);
    leaves2.set_interactivity(false);
    game_state.add_model(std::move(leaves2), leaves2.name());

    auto      flokati2 = Models::Model("assets/models/SimpleOldTownAssets/Flokati.obj", prefix + "flokati");
    glm::vec3 flokati_pos2 = room_offset2 + glm::vec3(5.0f, 0.0f, 0.0f);
    glm::mat4 flokati_tf2  = glm::translate(flokati2.get_local_transform(), flokati_pos2);
    flokati_tf2            = glm::scale(flokati_tf2, glm::vec3(5.5f, 1.0f, 5.5f));
    flokati2.set_local_transform(flokati_tf2);
    flokati2.set_interactivity(false);
    game_state.add_model(std::move(flokati2), flokati2.name());

    auto      bed2     = Models::Model("assets/models/SimpleOldTownAssets/Bed02.obj", prefix + "bed");
    glm::vec3 bed_pos2 = room_offset2 + glm::vec3(5.0f, 0.0f, 0.0f);
    glm::mat4 bed_tf2 = glm::translate(bed2.get_local_transform(), bed_pos2);
    bed_tf2            = glm::scale(bed_tf2, glm::vec3(1.2f, 1.2f, 1.2f));
    bed2.set_local_transform(bed_tf2);
    bed2.set_interactivity(false);
    game_state.add_model(std::move(bed2),prefix+"bed");

    auto page_pos2 = room_offset2 + glm::vec3(5.0f,0.0f,8.0f);
    scroll.add_instance_transform(glm::translate(scroll.get_local_transform(),page_pos2),"page-8");
    

    game_state.add_model(std::move(scroll), "page");
    game_state.add_model(std::move(wall), "wall");

    scene_manager.set_game_state(game_state);


    scene_manager.bind_handler_to_model("room-2-door", [](Game::SceneManager* scene_manager) {
        // TODO: fix it so it properly rotates in place like a door
        auto door_to_toggle = scene_manager->get_game_state()->find_model("room-2-door");
        if (!door_to_toggle) {
            throw std::runtime_error("Door Model not found");
        }
        static bool      isOpen      = false;
        static bool      initialized = false;
        static glm::mat4 closedXf, openXf;
        static auto      hinge = glm::vec3(-1.0f, 0.0f, 0.0f);

        if (!initialized) {
            initialized = true;
            closedXf    = door_to_toggle->get_local_transform();
            openXf      = glm::rotate(closedXf, glm::radians(-90.0f), {0.0f, 1.0f, 0.0f});
        }

        if (isOpen) {
            door_to_toggle->set_local_transform(closedXf);
        } else {
            door_to_toggle->set_local_transform(openXf);
        }
        isOpen = !isOpen;
        return true;
    });


    scene_manager.bind_handler_to_model("Door", [](Game::SceneManager* scene_manager) {
        // TODO: fix it so it properly rotates in place like a door
        auto door_to_toggle = scene_manager->get_game_state()->find_model("Door");
        if (!door_to_toggle) {
            throw std::runtime_error("Door Model not found");
        }
        static bool      isOpen      = false;
        static bool      initialized = false;
        static glm::mat4 closedXf, openXf;
        static auto      hinge = glm::vec3(-1.0f, 0.0f, 0.0f);

        if (!initialized) {
            initialized = true;
            closedXf    = door_to_toggle->get_local_transform();
            openXf      = glm::rotate(closedXf, glm::radians(-90.0f), {0.0f, 1.0f, 0.0f});
        }

        if (isOpen) {
            door_to_toggle->set_local_transform(closedXf);
        } else {
            door_to_toggle->set_local_transform(openXf);
        }
        isOpen = !isOpen;
        return true;
    });
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
