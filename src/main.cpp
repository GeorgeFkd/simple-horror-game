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

Model::Model model_from_obj_file(const std::string &obj_file,
                                 const std::string &label) {

  ObjectLoader::OBJLoader loader;
  loader.read_from_file(obj_file);
  std::cout << "For Model " << label << "\n";
  // loader.debug_dump();
  std::cout << "---------------------------";
  auto model = Model::Model(loader, label);
  return model;
}
enum class SurfaceType {
  Floor,
  Ceiling,
  WallFront,
  WallBack,
  WallLeft,
  WallRight
};
Model::Model repeating_tile(SurfaceType surface, float offset,
                            const Material &material) {
  const int TILE_WIDTH = 500;
  const int TILE_HEIGHT = 500;
  const float half_width = TILE_WIDTH / 2.0f;
  const float half_height = TILE_HEIGHT / 2.0f;
  const float repeat = 100.0f;

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

  return Model::Model(verts, normals, uvs, indices, material, label);
}
// Model::Model repeating_tile(SurfaceType surface, float offset) {
//     const int TILE_WIDTH = 512;
//     const int TILE_HEIGHT = 512;
//     const float half_width = TILE_WIDTH / 2.0f;
//     const float half_height = TILE_HEIGHT / 2.0f;
//     const float repeat = 50.0f;
//
//     std::vector<glm::vec3> verts;
//     std::vector<glm::vec2> uvs;
//     glm::vec3 normal;
//     std::string label;
//
//     switch (surface) {
//         case SurfaceType::Floor:
//             verts = {
//                 {-half_width, offset, -half_height},
//                 {-half_width, offset,  half_height},
//                 { half_width, offset,  half_height},
//                 { half_width, offset, -half_height}
//             };
//             label = "floor";
//             normal = glm::vec3(0, 1, 0);
//             break;
//
//         case SurfaceType::Ceiling:
//             verts = {
//                 {-half_width, offset, -half_height},
//                 {-half_width, offset,  half_height},
//                 { half_width, offset,  half_height},
//                 { half_width, offset, -half_height}
//             };
//             label = "ceiling";
//             normal = glm::vec3(0, -1, 0);
//             break;
//
//         case SurfaceType::WallFront:
//             verts = {
//                 {-half_width, -half_height, offset},
//                 {-half_width,  half_height, offset},
//                 { half_width,  half_height, offset},
//                 { half_width, -half_height, offset}
//             };
//             label = "wallfront";
//             normal = glm::vec3(0, 0, 1);
//             break;
//
//         case SurfaceType::WallBack:
//             verts = {
//                 { half_width, -half_height, offset},
//                 {-half_width, -half_height, offset},
//                 {-half_width,  half_height, offset},
//                 { half_width,  half_height, offset}
//             };
//             label = "wallback";
//             normal = glm::vec3(0, 0, -1);
//             break;
//
//         case SurfaceType::WallLeft:
//             verts = {
//                 {offset, -half_height,  half_width},
//                 {offset,  half_height,  half_width},
//                 {offset,  half_height, -half_width},
//                 {offset, -half_height, -half_width}
//             };
//             label = "wallleft";
//             normal = glm::vec3(-1, 0, 0);
//             break;
//
//         case SurfaceType::WallRight:
//             verts = {
//                 {offset, -half_height, -half_width},
//                 {offset,  half_height, -half_width},
//                 {offset,  half_height,  half_width},
//                 {offset, -half_height,  half_width}
//             };
//             label = "wallright";
//             normal = glm::vec3(1, 0, 0);
//             break;
//     }
//
//     uvs = {
//         {0, 0}, {0, repeat}, {repeat, repeat}, {repeat, 0}
//     };
//
//     std::vector<glm::vec3> normals(4, normal);
//     std::vector<GLuint> indices = {0, 1, 2, 0, 2, 3};
//
//     Material material;
//     material.Ka = glm::vec3(0.15f, 0.07f, 0.02f);
//     material.Kd = glm::vec3(0.59f, 0.29f, 0.00f);
//     material.Ks = glm::vec3(0.05f, 0.04f, 0.03f);
//     material.Ns = 16.0f;
//     material.d  = 1.0f;
//     material.illum = 2;
//
//     return Model::Model(verts, normals, uvs, indices, material,
//     std::move(label));
// }
int main() {
  // ─── Initialize SDL + OpenGL ──────────────────────────────────────────
  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window *window = SDL_CreateWindow(
      "Simple Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext glCtx = SDL_GL_CreateContext(window);

  glewInit();
  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, 1280, 720);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // ObjectLoader::OBJLoader cube_loader;
  // cube_loader.read_from_file("assets/models/test.obj");
  // ObjectLoader::OBJLoader lederliege;
  // lederliege.read_from_file("assets/models/lederliege.obj");
  // ObjectLoader::OBJLoader cottage_loader;
  // cottage_loader.read_from_file("assets/models/cottage_obj.obj");
  // ObjectLoader::OBJLoader sphere_loader;
  // sphere_loader.read_from_file("assets/models/light_sphere.obj");

  std::vector<std::string> shader_paths = {"assets/shaders/blinnphong.vert",
                                           "assets/shaders/blinnphong.frag"};
  std::vector<GLenum> shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  Shader blinnphong = Shader(shader_paths, shader_types, "blinn-phong");

  shader_paths = {"assets/shaders/depth_2d.vert",
                  "assets/shaders/depth_2d.frag"};
  shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  Shader depth_2d = Shader(shader_paths, shader_types, "depth_2d");

#ifdef DEBUG_DEPTH
  shader_paths = {"assets/shaders/depth_debug.vert",
                  "assets/shaders/depth_debug.frag"};
  shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
  Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
#endif

  shader_paths = {"assets/shaders/depth_cube.vert",
                  "assets/shaders/depth_cube.geom",
                  "assets/shaders/depth_cube.frag"};
  shader_types = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
  Shader depth_cube = Shader(shader_paths, shader_types, "depth_cube");


  auto right_light =
      model_from_obj_file("assets/models/light_sphere.obj", "Sphere");
  auto overhead_light =
      model_from_obj_file("assets/models/light_sphere.obj", "Overhead light");
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

  Light right_spotlight(
      LightType::SPOT, glm::vec3(5.0f, 1.5f, 0.0f), // position: to the right
      glm::vec3(-1.0f, 0.0f, 0.0f),                 // direction: pointing left
      glm::vec3(0.1f), glm::vec3(1.0f), glm::vec3(1.0f),
      glm::cos(glm::radians(95.0f)),  // inner cone
      glm::cos(glm::radians(125.0f)), // outer cone
      1024, 1024, 1.0f, 100.0f, 10.0f);

  Light overhead_spot(LightType::SPOT,
                      glm::vec3(0.0f, 15.0f, 0.0f), // above the object
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


    auto bed_position = glm::vec3(0.25f,0.0f,0.25f);
auto bed = model_from_obj_file("assets/models/SimpleOldTownAssets/Bed01.obj", "Bed");
    scene_manager.add_model(bed);


    glm::mat4 chair1_offset = glm::translate(glm::mat4(1.0f), bed_position + glm::vec3(-0.5f, 0.0f, 1.0f));
    auto chair1 = model_from_obj_file("assets/models/SimpleOldTownAssets/ChairCafeWhite01.obj", "Chair 1");
    chair1_offset = glm::rotate(chair1_offset, glm::radians(90.0f), glm::vec3(0, 1, 0)); // rotate around Y-axis
    chair1.set_local_transform(chair1_offset);
    scene_manager.add_model(chair1);

  
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
  float room_size = 12.5f;
    float room_height = 12.5f;
  auto floor = repeating_tile(SurfaceType::Floor, 0.0f, material);
  auto ceiling = repeating_tile(SurfaceType::Ceiling, room_height, material);
  auto wallF = repeating_tile(SurfaceType::WallFront, -room_size, material);
  auto wallB = repeating_tile(SurfaceType::WallBack, room_size, material);
  auto wallL = repeating_tile(SurfaceType::WallLeft, -room_size, material);
  auto wallR = repeating_tile(SurfaceType::WallRight, room_size, material);

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
  // ─── Create camera ───────────────────────────────────────────────────
  Camera::CameraObj camera(1280, 720);
  camera.set_position(glm::vec3{0.0f, 1.0f, 10.0f});
  // camera.set_direction(overhead_spot.get_direction());

#ifdef DEBUG_DEPTH
  float quadVertices[] = {// positions   // texCoords
                          -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                          0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                          -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                          1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};

  GLuint quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
#endif

  // ─── Main loop ───────────────────────────────────────────────────────
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
    }

    // 3) update camera movement (WASD/etc) once per frame
    last_camera_position = camera.get_position();
    camera.update(dt);
// 3.5) collision test
#ifndef DEBUG_DEPTH
    for (auto *model : scene_manager.get_models()) {
      if (camera.intersectSphereAABB(camera.get_position(), camera.get_radius(),
                                     model->get_aabbmin(),
                                     model->get_aabbmax())) {
        std::cout << "Collision with: " << model->name() << "\n";
        // problem is we start from inside the object
        // collision! revert to last safe position
        camera.set_position(last_camera_position);
        break;
      }
    }
#endif
    // 4) clear and render
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 view = camera.get_view_matrix();
    glm::mat4 proj = camera.get_projection_matrix();
    glm::mat4 vp = proj * view;
    scene_manager.set_spotlight(0, camera.get_position(),
                                camera.get_direction());
    // scene_manager.set_spotlight(2, camera.get_position(),
    // camera.get_direction());
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
    scene_manager.render(vp);
    // cube_model.draw(vp);
    SDL_GL_SwapWindow(window);
  }
  // ─── Cleanup ─────────────────────────────────────────────────────────
  SDL_GL_DeleteContext(glCtx);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
