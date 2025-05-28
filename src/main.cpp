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

#ifndef DEBUG_DEPTH
// #define DEBUG_DEPTH
#endif

using namespace Game;
using namespace Models;

enum class SurfaceType { Floor, Ceiling, WallFront, WallBack, WallLeft, WallRight };
struct
Model repeating_tile(SurfaceType surface, float offset, const Material& material) {
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

    return Model(verts, normals, texcoords, indices, std::move(label), material);
}

struct State {
    std::string_view closestModelLabel;
    float            closestModelDistance;
};

Model createFloor(float roomSize) {

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

    return Model(floor_verts, floor_normals, floor_uvs, floor_indices, std::move("Floor"), floor_material);
}
int main() {
    Camera::CameraObj  camera(1280, 720, glm::vec3(0.0f, 10.0f, 3.5f));
    Game::SceneManager scene_manager(1280, 720, camera);
    scene_manager.initialise_opengl_sdl();

    scene_manager.initialise_shaders();
    GameState gameState;

#ifdef DEBUG_DEPTH
    shader_paths       = {"assets/shaders/depth_debug.vert", "assets/shaders/depth_debug.frag"};
    shader_types       = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
#endif
    constexpr float ROOM_HEIGHT = 50.0f;
    constexpr float ROOM_WIDTH  = 60.0f;
    constexpr float ROOM_DEPTH  = ROOM_WIDTH;

    auto floor_model = createFloor(ROOM_WIDTH);
    // floor_model.init_instancing(6);
    // struct SurfaceConfig {
    //     glm::vec3 position;
    //     glm::vec3 rotation_axis;
    //     float     rotation_deg;
    // };
    // //all of the walls walls face the same problem
    // std::vector<SurfaceConfig> surfaces = {
    //     // Floor
    //     {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f},
    //
    //     // //Ceiling
    //     {{0.0f, ROOM_HEIGHT, 0.0f}, {1.0f, 0.0f, 0.0f}, 180.0f},
    //     //
    //     // // Back Wall
    //     {{0.0f, ROOM_HEIGHT / 2, -ROOM_DEPTH / 2}, {1.0f, 0.0f, 0.0f}, 90.0f},
    //     //
    //     // // Front Wall
    //     {{0.0f, ROOM_HEIGHT / 2, ROOM_DEPTH / 2}, {1.0f, 0.0f, 0.0f}, -90.0f},
    //     //
    //     // // Left Wall
    //     {{-ROOM_WIDTH / 2, ROOM_HEIGHT / 2, 0.0f}, {0.0f, 0.0f, 1.0f}, -90.0f},
    //     //
    //     // // Right Wall
    //     {{ROOM_WIDTH / 2, ROOM_HEIGHT / 2, 0.0f}, {0.0f, 0.0f, 1.0f}, 90.0f},
    // };
    //
    // for (const auto& face : surfaces) {
    //     glm::mat4 tf = glm::translate(glm::mat4(1.0f), face.position);
    //     if (glm::length(face.rotation_axis) > 0.0f) {
    //         tf = glm::rotate(tf, glm::radians(face.rotation_deg), face.rotation_axis);
    //     }
    //     floor_model.add_instance_transform(tf);
    // }

    floor_model.debug_dump();
    gameState.add_model(floor_model);

    auto scroll = Model("assets/models/scroll.obj", "page");
    scroll.init_instancing(6);
    glm::vec3 scroll_positions[6];
    scroll_positions[0] = {-29.0f, 0.0f, -29.0f};
    scroll_positions[1] = {-15.0f, 0.f, 20.0f};
    scroll_positions[2] = {-5.0f, 0.0f, -20.0f};
    scroll_positions[3] = {5.0f, 0.0f, 20.0f};
    scroll_positions[4] = {29.0f, 0.0f, -29.0f};
    scroll_positions[5] = {29.0f, 0.0f, 29.0f};

    scroll.set_interactivity(true);
    for (int i = 0; i < 6; i++)
        scroll.add_instance_transform(glm::translate(glm::mat4(1.0f), scroll_positions[i]),"-" + std::to_string(i));
    scroll.debug_dump();
    gameState.add_model(scroll);

    auto wall = Model("assets/models/SimpleOldTownAssets/OldHouseBrownWallLarge.obj", "Wall");
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

    wall.init_instancing(grid_rows * grid_columns * 2);

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

                wall.add_instance_transform(tf,"-h-" + std::to_string(row) + "-" + std::to_string(col));
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

                wall.add_instance_transform(tf,"-v-" + std::to_string(row) + "-" + std::to_string(col));

            }
        }
    };

    gameState.add_model(wall);

    auto right_light = Model("assets/models/light_sphere.obj", "Sphere");
    auto overhead_point_light_model =
        Model("assets/models/light_sphere.obj", "Overhead point light");
    auto  right_spot_light_model = Model("assets/models/light_sphere.obj", "Right spot light");
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
    gameState.add_light(flashlight);
    // gameState.add_light(right_spotlight);

#ifdef DEBUG_DEPTH
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
#endif

    scene_manager.set_game_state(gameState);
    scene_manager.on_interaction_with(
        "Bookcase", [](auto sceneMgr) { std::cout << "I live with only a chair on my side\n"; });

    for (size_t i = 0; i < 6; i++) {
        scene_manager.on_interaction_with("page-" + std::to_string(i), [i](SceneManager* sceneMgr) {
            std::cout << "You found page: " << i << "\n";
            auto m = sceneMgr->get_game_state()->findModel("page");
            sceneMgr->remove_instanced_model_at(m, "-" + std::to_string(i));
        });
    }
    scene_manager.run_game_loop();

    return 0;
}
