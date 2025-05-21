// main.cpp
#include "Camera.h"
#include "Light.h"
#include "SceneManager.h"
#include "Shader.h"
#include <GL/glew.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#ifndef DEBUG_DEPTH
// #define DEBUG_DEPTH
#endif

using Models::Model;

enum class SurfaceType { Floor, Ceiling, WallFront, WallBack, WallLeft, WallRight };
Models::Model repeating_tile(SurfaceType surface, float offset, const Material& material,
                             float repeat) {
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

    texcoords = {{0, 0}, {0, repeat}, {repeat, repeat}, {repeat, 0}};

    std::vector<glm::vec3> normals(4, normal);
    std::vector<GLuint>    indices = {0, 1, 2, 0, 2, 3};

    return Models::Model(verts, normals, texcoords, indices, label, material);
}

struct State {
    std::string_view closestModelLabel;
    float            closestModelDistance;
};

using namespace Game;
int main() {
    Camera::CameraObj camera(1280, 720);
    Game::SceneManager scene_manager(1280, 720, camera);
    scene_manager.initialiseOpenGL_SDL();

    scene_manager.initialiseShaders();
    GameState gameState;

#ifdef DEBUG_DEPTH
    shader_paths       = {"assets/shaders/depth_debug.vert", "assets/shaders/depth_debug.frag"};
    shader_types       = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
#endif

    std::vector<glm::vec3> floor_verts = {
        {-10.0f, 0.0f, -10.0f}, {-10.0f, 0.0f, 10.0f}, {10.0f, 0.0f, 10.0f}, {10.0f, 0.0f, -10.0f}};
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

    Models::Model floor(floor_verts, floor_normals, floor_uvs, floor_indices, "Floor",
                        floor_material);

    auto right_light = Models::Model("assets/models/light_sphere.obj", "Sphere");
    auto overhead_point_light_model =
        Models::Model("assets/models/light_sphere.obj", "Overhead point light");
    auto right_spot_light_model =
        Models::Model("assets/models/light_sphere.obj", "Right spot light");
    // hi
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
    Light right_spot_light(LightType::SPOT, glm::vec3(5.0f, 1.5f, 0.0f), // position: to the right
                           glm::vec3(1.0f, 0.0f, -1.0f),                 // direction: pointing left
                           glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f),
                           glm::cos(glm::radians(10.0f)), // inner cone
                           glm::cos(glm::radians(30.0f)), // outer cone
                           2048, 2048, 1.0f, 10.0f, 10.0f, 1.0f, 0.35f, 0.44f, 1.0f, 1.0f);

    Light overhead_point_light(LightType::POINT, glm::vec3(0.0f, 5.0f, 0.0f), // above the object
                               glm::vec3(0.0f, -1.0f, 0.0f), // pointing straight down
                               glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f),
                               glm::cos(glm::radians(10.0f)), // inner cone
                               glm::cos(glm::radians(30.0f)), // outer cone
                               2048, 2048, 0.1f, 10.0f, 10.0f, 1.0f, 0.35f, 0.44f, 1.0f, 1.0f);

    glm::vec3 overhead_light_spot = glm::vec3(15.0f, 5.0f, -20.0f);
    overhead_point_light.set_position(overhead_light_spot);
    overhead_point_light_model.set_local_transform(
        glm::translate(glm::mat4(1.0f), overhead_point_light.get_position()));

    glm::vec3 right_light_spot = glm::vec3(15.0f, 2.0f, -25.0f);
    right_spot_light.set_position(right_light_spot);
    right_spot_light_model.set_local_transform(
        glm::translate(glm::mat4(1.0f), right_spot_light.get_position()));

    floor.set_local_transform(glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, -0.01f, -20.0f)));

    // gameState.add_model(overhead_point_light_model);
    // gameState.add_model(right_spot_light_model);
    gameState.add_model(floor);
    // scene_manager.add_light(overhead_point_light);
    gameState.add_light(flashlight);
    gameState.add_light(right_spot_light);

    glm::vec3 bed_position = glm::vec3(15.0f, 0.0f, -20.0f);
    glm::mat4 bed_offset   = glm::translate(glm::mat4(1.0f), bed_position);

    // glm::vec3 right_spot_dir = glm::normalize((bed_position + glm::vec3(0.0f, 0.0f, -6.0f)) -
    // right_spot_light.get_position());
    glm::vec3 right_spot_dir = glm::normalize(bed_position - right_spot_light.get_position());
    right_spot_light.set_direction(right_spot_dir);

    glm::vec3 overhead_spot_dir =
        glm::normalize(bed_position - overhead_point_light.get_position());
    overhead_point_light.set_direction(overhead_spot_dir);

    auto bed = Model("assets/models/SimpleOldTownAssets/Bed01.obj", "Bed");
    bed.set_local_transform(bed_offset);
    bed.set_interactivity(true);
    gameState.add_model(bed);

    auto chair =
        Models::Model("assets/models/SimpleOldTownAssets/ChairCafeWhite01.obj", "Cafe Chair");

    constexpr int chair_count = 1000;
    chair.init_instancing(chair_count);

    const int   grid_width    = 40; // 40 × 25 = 1000
    const int   grid_height   = 25;
    const float chair_spacing = 2.5f;

    int placed = 0;
    for (int row = 0; row < grid_height && placed < chair_count; ++row) {
        for (int col = 0; col < grid_width && placed < chair_count; ++col) {
            glm::vec3 offset =
                bed_position + glm::vec3((col - grid_width / 2) * chair_spacing, 0.0f,
                                         (row - grid_height / 2) * chair_spacing);

            glm::mat4 transform = glm::translate(glm::mat4(1.0f), offset);

            // Optional: rotate every third chair
            if ((row + col) % 3 == 0) {
                transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(0, 1, 0));
            }

            chair.add_instance_transform(transform);
            ++placed;
        }
    }

    gameState.add_model(chair);

    glm::mat4 bookcase_offset =
        glm::translate(glm::mat4(1.0f), bed_position + glm::vec3(0.0f, 0.0f, -6.0f));
    auto bookcase = Models::Model("assets/models/SimpleOldTownAssets/BookCase01.obj", "Bookcase");
    bookcase.set_local_transform(bookcase_offset);
    bookcase.set_interactivity(true);
    gameState.add_model(bookcase);


    Material material;
    {
        material.Ka     = glm::vec3(0.15f, 0.07f, 0.02f);
        material.Kd     = glm::vec3(0.59f, 0.29f, 0.00f);
        material.Ks     = glm::vec3(0.05f, 0.04f, 0.03f);
        material.Ns     = 16.0f;
        material.d      = 1.0f;
        material.illum  = 2;
        auto   texpath  = "assets/textures/Wood092_1K-JPG/Wood092_1K-JPG_Color.jpg";
        GLuint texture  = ObjectLoader::load_texture_from_file(texpath);
        material.map_Kd = texpath;
        material.tex_Kd = texture;
    }

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
    scene_manager.runGameLoop();


    return 0;
}
