
// main.cpp
#include <GL/glew.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "Camera.h"
#include "Light.h"
#include "OBJLoader.h"
#include "SceneManager.h"
#include "Shader.h"

#ifndef DEBUG_DEPTH
// #define DEBUG_DEPTH
#endif


enum class SurfaceType {
  Floor,
  Ceiling,
  WallFront,
  WallBack,
  WallLeft,
  WallRight
};
Model::Model repeating_tile(SurfaceType surface, float offset,
                            const Material &material, float repeat) {
  constexpr int TILE_WIDTH = 50;
  constexpr int TILE_HEIGHT = 50;
  constexpr float half_width = TILE_WIDTH / 2.0f;
  constexpr float half_height = TILE_HEIGHT / 2.0f;

  std::vector<glm::vec3> verts;
  std::vector<glm::vec2> uvs;
  glm::vec3 normal;
  std::string label;

  switch (surface) {
  case SurfaceType::Floor:
    verts = {{-half_width, offset, -half_height},
             {-half_width, offset, half_height},
             {half_width, offset, half_height},
             {half_width, offset, -half_height}};
    label = "floor";
    normal = glm::vec3(0, 1, 0);
    break;

  case SurfaceType::Ceiling:
    verts = {{-half_width, offset, -half_height},
             {-half_width, offset, half_height},
             {half_width, offset, half_height},
             {half_width, offset, -half_height}};
    label = "ceiling";
    normal = glm::vec3(0, -1, 0);
    break;

  case SurfaceType::WallFront:
    verts = {{-half_width, -half_height, offset},
             {-half_width, half_height, offset},
             {half_width, half_height, offset},
             {half_width, -half_height, offset}};
    label = "wallfront";
    normal = glm::vec3(0, 0, 1);
    break;

  case SurfaceType::WallBack:
    verts = {{half_width, -half_height, offset},
             {-half_width, -half_height, offset},
             {-half_width, half_height, offset},
             {half_width, half_height, offset}};
    label = "wallback";
    normal = glm::vec3(0, 0, -1);
    break;

  case SurfaceType::WallLeft:
    verts = {{offset, -half_height, half_width},
             {offset, half_height, half_width},
             {offset, half_height, -half_width},
             {offset, -half_height, -half_width}};
    label = "wallleft";
    normal = glm::vec3(1, 0, 0);
    break;

  case SurfaceType::WallRight:
    verts = {{offset, -half_height, -half_width},
             {offset, half_height, -half_width},
             {offset, half_height, half_width},
             {offset, -half_height, half_width}};
    label = "wallright";
    normal = glm::vec3(-1, 0, 0);
    break;
  }

  uvs = {{0, 0}, {0, repeat}, {repeat, repeat}, {repeat, 0}};

  std::vector<glm::vec3> normals(4, normal);
  std::vector<GLuint> indices = {0, 1, 2, 0, 2, 3};

  return Model::Model(verts, normals, uvs, indices,label, material );
}




static unsigned int INITIAL_WIDTH = 1280;
static unsigned int INITIAL_HEIGHT = 720;



int main() {
  // ─── Initialize SDL + OpenGL ──────────────────────────────────────────
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window *window = SDL_CreateWindow(
        "Old room", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, INITIAL_WIDTH,INITIAL_HEIGHT,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext glCtx = SDL_GL_CreateContext(window);

  glewInit();
  glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, INITIAL_WIDTH,INITIAL_HEIGHT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  std::vector<std::string> shader_paths = {"assets/shaders/blinnphong.vert",
                                           "assets/shaders/blinnphong.frag"};
  std::vector<GLenum> shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  Shader blinnphong = Shader(shader_paths, shader_types, "blinn-phong");

  shader_paths = {"assets/shaders/depth_2d.vert",
                  "assets/shaders/depth_2d.frag"};
  shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  Shader depth_2d = Shader(shader_paths, shader_types, "depth_2d");


  shader_paths = {"assets/shaders/depth_cube.vert",
                  "assets/shaders/depth_cube.geom",
                  "assets/shaders/depth_cube.frag"};
  shader_types = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
  Shader depth_cube = Shader(shader_paths, shader_types, "depth_cube");

  auto right_light =
        Model::Model("assets/models/light_sphere.obj", "Sphere");
  auto overhead_light =
        Model::Model("assets/models/light_sphere.obj", "Overhead light");
  // hi
  Light flashlight(LightType::SPOT,
                   glm::vec3(0.0f),               // position
                   glm::vec3(0.0f, 0.0f, -1.0f),  // direction
                   glm::vec3(0.1f),               // ambient
                   glm::vec3(1.0f),               // diffuse
                   glm::vec3(1.0f),               // specular
                   glm::cos(glm::radians(12.5f)), // cutoff
                   glm::cos(glm::radians(17.5f)), // outer cutoff
                   1024, 1024, 1.0f, 100.0f, 10.0f);

  auto right_spotlight_pos = glm::vec3(0.0f, 5.0f, 0.0f);
  Light right_spotlight(
      LightType::SPOT,
      right_spotlight_pos,          // position: to the right
      glm::vec3(-1.0f, 0.0f, 0.0f), // direction: pointing left
      glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f),
      glm::cos(glm::radians(95.0f)),  // inner cone
      glm::cos(glm::radians(125.0f)), // outer cone
      1024, 1024, 1.0f, 100.0f, 10.0f);

  glm::vec3 overhead_light_pos = glm::vec3(0.0, 5.0f, 0.0f);
  Light overhead_spot(LightType::SPOT,
                      overhead_light_pos,           // above the object
                      glm::vec3(0.0f, -1.0f, 0.0f), // pointing straight down
                      glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f),
                      glm::cos(glm::radians(95.0f)),  // inner cone
                      glm::cos(glm::radians(125.0f)), // outer cone
                      1024, 1024, 1.0f, 100.0f, 10.0f);

  right_light.set_local_transform(
      glm::translate(glm::mat4(1.0f), right_spotlight.get_position()));
  overhead_light.set_local_transform(
      glm::translate(glm::mat4(1.0f), overhead_spot.get_position()));

  SceneManager::SceneManager scene_manager(1280, 720);
  scene_manager.add_shader(blinnphong);
  scene_manager.add_shader(depth_2d);
  scene_manager.add_shader(depth_cube);

  auto bed_position = glm::vec3(0.25f, 0.0f, 0.25f);
  auto bed =
      Model::Model("assets/models/SimpleOldTownAssets/Bed01.obj", "Bed");
  scene_manager.add_model(bed);

  #define OFFSET(X, Y) glm::translate(glm::mat4(1.0f), X + Y)

  glm::mat4 chair1_offset = OFFSET(bed_position, glm::vec3(-0.5f, 0.0f, 1.0f));
  auto chair1 = Model::Model(
      "assets/models/SimpleOldTownAssets/ChairCafeWhite01.obj", "Chair 1");
  chair1_offset = glm::rotate(chair1_offset, glm::radians(90.0f),
                              glm::vec3(0, 1, 0)); // rotate around Y-axis
  chair1.set_local_transform(chair1_offset);
  scene_manager.add_model(chair1);

  std::string MODELS_FOLDER = "assets/models/SimpleOldTownAssets/";
  std::string bookcase_obj = "BookCase01.obj";
  auto bookcase_file = MODELS_FOLDER + bookcase_obj;

  glm::mat4 bookcase_offset =
      OFFSET(bed_position, glm::vec3(1.0f, 0.0f, -3.5f));
  auto bookcase = Model::Model(bookcase_file, "bookcase1");
  bookcase.set_local_transform(bookcase_offset);
  scene_manager.add_model(bookcase);

  glm::mat4 bookcase2_offset =
      OFFSET(bed_position, glm::vec3(-0.20f, 0.0f, -3.5f));
  auto bookcase2 = Model::Model(bookcase_file, "bookcase");
  bookcase2.set_local_transform(bookcase2_offset);
  scene_manager.add_model(bookcase2);

  glm::mat4 bookcase3_offset =
      OFFSET(bed_position, glm::vec3(-1.4f, 0.0f, -3.5f));
  auto bookcase3 = Model::Model(bookcase_file, "bookcase");
  bookcase3.set_local_transform(bookcase3_offset);
  scene_manager.add_model(bookcase3);

  glm::mat4 bookcase4_offset =
      OFFSET(bed_position, glm::vec3(1.4f, 0.0f, -2.7f));
  bookcase4_offset = glm::rotate(bookcase4_offset, glm::radians(-90.0f),
                                 glm::vec3(0, 1, 0)); // rotate around Y-axis
  auto bookcase4 = Model::Model(bookcase_file, "bookcase");
  bookcase4.set_local_transform(bookcase4_offset);
  scene_manager.add_model(bookcase4);

  glm::mat4 table_offset = OFFSET(bed_position, glm::vec3(0.0f, 0.0f, -2.0f));
  auto table = Model::Model(MODELS_FOLDER + "TableSmall1.obj", "Table");
  table.set_local_transform(table_offset);
  scene_manager.add_model(table);

  glm::mat4 tablechair_offset =
      OFFSET(bed_position, glm::vec3(-1.0f, 0.0f, -2.0f));
  auto tablechair =
      Model::Model(MODELS_FOLDER + "ChairCafeWhite01.obj", "TableChair");
  tablechair.set_local_transform(tablechair_offset);
  scene_manager.add_model(tablechair);

  auto overhead_light_model =
      Model::Model("assets/models/light_sphere.obj", "lamp");
  glm::mat4 overhead_light_offset =
      OFFSET(glm::vec3(0.0f, 0.0f, 0.0f), overhead_light_pos);
  overhead_light_model.set_local_transform(overhead_light_offset);
  scene_manager.add_model(overhead_light_model);

  auto rightlight_model =
      Model::Model("assets/models/light_sphere.obj", "lamp2");
  glm::mat4 rightlight_offset =
      OFFSET(glm::vec3(0.0f, 0.0f, -2.0f), right_spotlight_pos);
  rightlight_model.set_local_transform(rightlight_offset);
  scene_manager.add_model(rightlight_model);

  auto carpet = Model::Model(MODELS_FOLDER + "Flokati.obj", "carpet");
  glm::mat4 carpet_offset =
      OFFSET(glm::vec3(0.0f, 0.0f, -1.85f), glm::vec3(0.0f));
  carpet.set_local_transform(carpet_offset);
  scene_manager.add_model(carpet);

  
  auto pot = Model::Model(MODELS_FOLDER + "PotNtural.obj", "pot");
  glm::mat4 pot_offset = OFFSET(bed_position, glm::vec3(-6.0f, 0, 3.0f));
  pot.set_local_transform(pot_offset);
  scene_manager.add_model(pot);

  auto pot2 = Model::Model(MODELS_FOLDER + "PotNtural.obj", "pot1");
  glm::mat4 pot2_offset = OFFSET(bed_position, glm::vec3(-5.5f, 0, 3.0f));
  pot2.set_local_transform(pot2_offset);
  scene_manager.add_model(pot2);

  auto pot3 = Model::Model(MODELS_FOLDER + "PotNtural.obj", "pot2");
  glm::mat4 pot3_offset = OFFSET(bed_position, glm::vec3(-5.0f, 0, 3.0f));
  pot3.set_local_transform(pot3_offset);
  scene_manager.add_model(pot3);

  auto pot4 = Model::Model(MODELS_FOLDER + "PotNtural.obj", "pot3");
  glm::mat4 pot4_offset = OFFSET(bed_position, glm::vec3(-4.5f, 0, 3.0f));
  pot4.set_local_transform(pot4_offset);
  scene_manager.add_model(pot4);

  auto watering_can =
      Model::Model(MODELS_FOLDER + "watering-can.obj", "watering-can");
  glm::mat4 watering_can_offset =
      OFFSET(bed_position, glm::vec3(-4.0f, 0.0f, 3.5f));
  watering_can.set_local_transform(watering_can_offset);
  scene_manager.add_model(watering_can);

  auto plant = Model::Model(MODELS_FOLDER + "leaves.obj", "leaves");
  glm::mat4 plant_offset = OFFSET(bed_position, glm::vec3(-4.0f, 0.0f, 3.0f));
  plant.set_local_transform(plant_offset);
  scene_manager.add_model(plant);

  auto dining =
      Model::Model(MODELS_FOLDER + "dining-place.obj", "dining-place");
  glm::mat4 dining_offset = OFFSET(bed_position, glm::vec3(-5.5f, 0.0f, -4.5f));
  dining.set_local_transform(dining_offset);
  scene_manager.add_model(dining);

  auto wall_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall_large-place");
  glm::mat4 wall_large_offset =
      OFFSET(bed_position, glm::vec3(-3.5f, 0.0f, -6.5f));
  wall_large.set_local_transform(wall_large_offset);
  scene_manager.add_model(wall_large);
  auto wall2_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall2_large_offset =
      OFFSET(bed_position, glm::vec3(-3.5f, 0.0f, 6.5f));
  wall2_large.set_local_transform(wall2_large_offset);
  scene_manager.add_model(wall2_large);

  auto wall3_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall3_large_offset =
      OFFSET(bed_position, glm::vec3(-3.5f, 0.0f, 3.5f));
  wall3_large.set_local_transform(wall3_large_offset);
  scene_manager.add_model(wall3_large);
  auto wall4_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall4_large_offset =
      OFFSET(bed_position, glm::vec3(-3.5f, 0.0f, -3.5f));
  wall4_large.set_local_transform(wall4_large_offset);
  scene_manager.add_model(wall4_large);
  auto wall5_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall5_large_offset =
      OFFSET(bed_position, glm::vec3(-3.5f, 0.0f, -1.5f));
  wall5_large.set_local_transform(wall5_large_offset);
  scene_manager.add_model(wall5_large);

  auto wall6_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall_large-place");
  glm::mat4 wall6_large_offset =
      OFFSET(bed_position, glm::vec3(2.5f, 0.0f, -6.5f));
  wall6_large.set_local_transform(wall6_large_offset);
  scene_manager.add_model(wall6_large);
  auto wall7_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall7_large_offset =
      OFFSET(bed_position, glm::vec3(2.5f, 0.0f, 6.5f));
  wall7_large.set_local_transform(wall7_large_offset);
  scene_manager.add_model(wall7_large);

  auto wall8_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall8_large_offset =
      OFFSET(bed_position, glm::vec3(2.5f, 0.0f, 3.5f));
  wall8_large.set_local_transform(wall8_large_offset);
  scene_manager.add_model(wall8_large);
  auto wall9_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall9_large_offset =
      OFFSET(bed_position, glm::vec3(2.5f, 0.0f, -3.5f));
  wall9_large.set_local_transform(wall9_large_offset);
  scene_manager.add_model(wall9_large);
  auto wall10_large = Model::Model(
      MODELS_FOLDER + "OldHouseBrownWallLarge.obj", "wall2_large-place");
  glm::mat4 wall10_large_offset =
      OFFSET(bed_position, glm::vec3(2.5f, 0.0f, -1.5f));
  wall10_large.set_local_transform(wall10_large_offset);
  scene_manager.add_model(wall10_large);
 
  Material material;
  {
    material.Ka = glm::vec3(0.15f, 0.07f, 0.02f);
    material.Kd = glm::vec3(0.59f, 0.29f, 0.00f);
    material.Ks = glm::vec3(0.05f, 0.04f, 0.03f);
    material.Ns = 16.0f;
    material.d = 1.0f;
    material.illum = 2;
    auto texpath = "assets/textures/Wood092_1K-JPG/Wood092_1K-JPG_Color.jpg";
    GLuint texture = ObjectLoader::load_texture_from_file(texpath);
    material.map_Kd = texpath;
    material.tex_Kd = texture;
  }

  Material materialRoomWall;
  {
    materialRoomWall.Ka = glm::vec3(0.15f, 0.07f, 0.02f);
    materialRoomWall.Kd = glm::vec3(0.59f, 0.29f, 0.00f);
    materialRoomWall.Ks = glm::vec3(0.05f, 0.04f, 0.03f);
    materialRoomWall.Ns = 16.0f;
    materialRoomWall.d = 1.0f;
    materialRoomWall.illum = 2;
    auto texpath = "assets/models/SimpleOldTown/WallBrown.jpg";
    GLuint texture = ObjectLoader::load_texture_from_file(texpath);
    materialRoomWall.map_Kd = texpath;
    materialRoomWall.tex_Kd = texture;
  }

  float room_size = 8.0f;
  float room_height = 6.5f;


  auto wallsRepeat = 5.0f;
  auto floor = repeating_tile(SurfaceType::Floor, 0.0f, material, wallsRepeat);
  auto ceiling =
      repeating_tile(SurfaceType::Ceiling, room_height, material, wallsRepeat);
  auto wallF =
      repeating_tile(SurfaceType::WallFront, -room_size, material, wallsRepeat);
  auto wallB =
      repeating_tile(SurfaceType::WallBack, room_size, material, wallsRepeat);
  auto wallL =
      repeating_tile(SurfaceType::WallLeft, -room_size, material, wallsRepeat);
  auto wallR =
      repeating_tile(SurfaceType::WallRight, room_size, material, wallsRepeat);

  {
    scene_manager.add_model(floor);
    scene_manager.add_model(ceiling);
    scene_manager.add_model(wallF);
    scene_manager.add_model(wallB);
    scene_manager.add_model(wallL);
    scene_manager.add_model(wallR);
  }

  {
    scene_manager.add_light(flashlight);
    scene_manager.add_light(right_spotlight);
    scene_manager.add_light(overhead_spot);
  }
  Camera::CameraObj camera(1280, 720);
  camera.set_position(glm::vec3{0.0f, 1.0f, 5.0f});

bool running = true;
  Uint64 lastTicks = SDL_GetPerformanceCounter();

  glm::vec3 last_camera_position;
  while (running) {
    // 1) compute Δt
    Uint64 now = SDL_GetPerformanceCounter();
    float dt = float(now - lastTicks) / float(SDL_GetPerformanceFrequency());
    lastTicks = now;
    // 2) handle all pending SDL events
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) {
        running = false;
      }
      // feed mouse/window events to the camera
      camera.process_input(ev);
      // adjust the GL viewport on resize
      if (ev.type == SDL_WINDOWEVENT &&
          ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        int w = ev.window.data1, h = ev.window.data2;
        glViewport(0, 0, w, h);
      }
    glm::vec3 last_camera_position;
    while (running){
        // 1) compute Δt
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = float(now - lastTicks) / float(SDL_GetPerformanceFrequency());
        lastTicks = now;
        // 2) handle all pending SDL events
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                running = false;
            }
            // feed mouse/window events to the camera
            camera.process_input(ev);
            // adjust the GL viewport on resize
            if (ev.type == SDL_WINDOWEVENT &&
                ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                int w = ev.window.data1,
                    h = ev.window.data2;
                glViewport(0, 0, w, h);
            }
        }

        // 3) update camera movement (WASD/etc) once per frame
        last_camera_position = camera.get_position();
        camera.update(dt);
        // 3.5) collision test
        #ifndef DEBUG_DEPTH
        for (auto* model : scene_manager.get_models()) {
            if (model->is_instanced()) {
                // loop each instance’s box
                for (size_t i = 0; i < model->get_instance_count(); ++i) {
                    if ( camera.intersectSphereAABB(
                        camera.get_position(),
                        camera.get_radius(),
                        model->get_instance_aabb_min(i),
                        model->get_instance_aabb_max(i)) )
                    {
                    camera.set_position(last_camera_position);
                    goto collision_done;
                    }
                }
            }
            else {
                // single AABB path
                if ( camera.intersectSphereAABB(
                        camera.get_position(),
                        camera.get_radius(),
                        model->get_aabbmin(),
                        model->get_aabbmax()) )
                {
                    camera.set_position(last_camera_position);
                    break;
                }
            }
        }
        collision_done:;
        #endif
        // 4) clear and render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 proj = camera.get_projection_matrix();
        glm::mat4 vp = proj * view;
        //float forward_offset = 0.5f;
        float right_offset = 0.4f;
        //glm::vec3 offset = right_offset * camera.get_right() + forward_offset * camera.get_direction();
        glm::vec3 offset = right_offset * camera.get_right();
        flashlight.set_position(camera.get_position() + offset);
        flashlight.set_direction(camera.get_direction());
        scene_manager.render_depth_pass();
    #ifdef DEBUG_DEPTH
        depth_debug.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, flashlight.get_depth_texture());
        glUniform1i(depth_debug.get_uniform_location("depthMap"), 0);
        glUniform1f(depth_debug.get_uniform_location("near_plane"), 1.0f);
        glUniform1f(depth_debug.get_uniform_location("far_plane"), 100.0f);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    #endif
        scene_manager.render(view, proj);
        // cube_model.draw(vp);
        SDL_GL_SwapWindow(window);
    }
    }
  }   
  // ─── Cleanup ─────────────────────────────────────────────────────────
  SDL_GL_DeleteContext(glCtx);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;

}
