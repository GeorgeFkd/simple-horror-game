// main.cpp
#include <SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "OBJLoader.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Shader.h"
#include "Light.h"

#ifndef DEBUG_DEPTH
//#define DEBUG_DEPTH
#endif



Model::Model model_from_obj_file(const std::string& obj_file,const std::string& label) {

    ObjectLoader::OBJLoader loader;
    loader.read_from_file(obj_file);
    std::cout << "For Model " << label << "\n";
    //loader.debug_dump();
    std::cout << "---------------------------";
    auto model = Model::Model(loader,label);
    return model;
}


int main() {
    // ─── Initialize SDL + OpenGL ──────────────────────────────────────────
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window *window = SDL_CreateWindow(
        "Simple Cube",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext glCtx = SDL_GL_CreateContext(window);

    glewInit();
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 1280, 720);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ObjectLoader::OBJLoader cube_loader;
    cube_loader.read_from_file("assets/models/test.obj");
    //cube_loader.debug_dump();
    //
    ObjectLoader::OBJLoader lederliege;
    lederliege.read_from_file("assets/models/lederliege.obj");
    //lederliege.debug_dump();
    //
    ObjectLoader::OBJLoader cottage_loader;
    cottage_loader.read_from_file("assets/models/cottage_obj.obj");
    //cottage_loader.debug_dump();
    //
    ObjectLoader::OBJLoader sphere_loader;
    sphere_loader.read_from_file("assets/models/light_sphere.obj");

    std::vector<std::string> shader_paths = {"assets/shaders/blinnphong.vert", "assets/shaders/blinnphong.frag"};
    std::vector<GLenum> shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader blinnphong = Shader(shader_paths, shader_types, "blinn-phong");

    shader_paths = {"assets/shaders/depth_2d.vert", "assets/shaders/depth_2d.frag"};
    shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_2d = Shader(shader_paths, shader_types, "depth_2d");

    #ifdef DEBUG_DEPTH
    shader_paths = {"assets/shaders/depth_debug.vert", "assets/shaders/depth_debug.frag"};
    shader_types = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_debug = Shader(shader_paths, shader_types, "depth_debug");
    #endif

    shader_paths = {"assets/shaders/depth_cube.vert", "assets/shaders/depth_cube.geom", "assets/shaders/depth_cube.frag"};
    shader_types = {GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};
    Shader depth_cube = Shader(shader_paths, shader_types, "depth_cube");

    //Model::Model cube(cube_loader);
    //Model::Model couch(lederliege);
    //Model::Model right_light(sphere_loader);
    //Model::Model overhead_light(sphere_loader);

    std::vector<glm::vec3> floor_verts = {
        {-10.0f, 0.0f, -10.0f},
        {-10.0f, 0.0f,  10.0f},
        { 10.0f, 0.0f,  10.0f},
        { 10.0f, 0.0f, -10.0f}
    };
    std::vector<glm::vec3> floor_normals(4, glm::vec3(0, 1, 0));
    std::vector<glm::vec2> floor_uvs = {
        {0, 0}, {0, 1}, {1, 1}, {1, 0}
    };
    std::vector<GLuint> floor_indices = { 0, 1, 2, 0, 2, 3 };

    Material floor_material;
    floor_material.Ka = glm::vec3(0.15f, 0.07f, 0.02f);         // dark ambient
    floor_material.Kd = glm::vec3(0.59f, 0.29f, 0.00f);         // brown diffuse
    floor_material.Ks = glm::vec3(0.05f, 0.04f, 0.03f);         // small specular
    floor_material.Ns = 16.0f;                                 // shininess
    floor_material.d  = 1.0f;                                  // opacity
    floor_material.illum = 2;                                  // standard Phong

    Model::Model floor(floor_verts, floor_normals, floor_uvs, floor_indices, floor_material);

    auto right_light = model_from_obj_file("assets/models/light_sphere.obj", "Sphere");
    auto overhead_point_light_model = model_from_obj_file("assets/models/light_sphere.obj","Overhead point light");
    auto right_spot_light_model = model_from_obj_file("assets/models/light_sphere.obj","Right spot light");
    //hi
    Light flashlight(
        LightType::SPOT,
        glm::vec3(0.0f),               // position
        glm::vec3(0.0f, 0.0f, -1.0f),  // direction
        glm::vec3(0.1f),               // ambient
        glm::vec3(1.0f),               // diffuse
        glm::vec3(1.0f),               // specular
        glm::cos(glm::radians(10.0f)), // cutoff
        glm::cos(glm::radians(20.0f)), // outer cutoff
        1280,
        720,
        1.0f,
        50.0f,
        10.0f);

    Light right_spot_light(
        LightType::SPOT,
        glm::vec3(5.0f, 1.5f, 0.0f),  // position: to the right
        glm::vec3(1.0f, 0.0f, -1.0f), // direction: pointing left
        glm::vec3(0.1f),
        glm::vec3(1.0f),
        glm::vec3(1.0f),
        glm::cos(glm::radians(10.0f)), // inner cone
        glm::cos(glm::radians(30.0f)), // outer cone
        1024, 1024,
        1.0f, 100.0f,
        10.0f);

    Light overhead_point_light(
        LightType::POINT,
        glm::vec3(0.0f, 5.0f, 0.0f),  // above the object
        glm::vec3(0.0f, -1.0f, 0.0f), // pointing straight down
        glm::vec3(0.1f),
        glm::vec3(1.0f),
        glm::vec3(1.0f),
        glm::cos(glm::radians(15.0f)), // inner cone
        glm::cos(glm::radians(25.0f)), // outer cone
        1024, 1024,
        1.0f, 100.0f,
        10.0f);


    glm::vec3 overhead_light_spot = glm::vec3(15.0f, 5.0f, -20.0f);
    overhead_point_light.set_position(overhead_light_spot);
    overhead_point_light_model.set_local_transform(glm::translate(glm::mat4(1.0f), overhead_point_light.get_position()));

    glm::vec3 right_light_spot = glm::vec3(15.0f, 2.0f, -25.0f);
    right_spot_light.set_position(right_light_spot);
    right_spot_light_model.set_local_transform(glm::translate(glm::mat4(1.0f), right_spot_light.get_position()));

    floor.set_local_transform(glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, 0.0f, -20.0f)));


    SceneManager::SceneManager scene_manager(1280, 720);
    scene_manager.add_shader(blinnphong);
    scene_manager.add_shader(depth_2d);
    scene_manager.add_shader(depth_cube);
    //scene_manager.add_model(overhead_point_light_model);
    //scene_manager.add_model(right_spot_light_model);
    scene_manager.add_model(floor);
    //scene_manager.add_light(overhead_point_light);
    scene_manager.add_light(right_spot_light);
    //scene_manager.add_light(flashlight);


    glm::vec3 bed_position = glm::vec3(15.0f, 0.0f, -20.0f);
    glm::mat4 bed_offset = glm::translate(glm::mat4(1.0f), bed_position);

    //glm::vec3 right_spot_dir = glm::normalize((bed_position + glm::vec3(0.0f, 0.0f, -6.0f)) - right_spot_light.get_position());
    glm::vec3 right_spot_dir = glm::normalize(bed_position - right_spot_light.get_position());
    right_spot_light.set_direction(right_spot_dir);


    auto bed = model_from_obj_file("assets/models/SimpleOldTownAssets/Bed01.obj", "Bed");
    bed.set_local_transform(bed_offset);
    scene_manager.add_model(bed);

    glm::mat4 chair1_offset = glm::translate(glm::mat4(1.0f), bed_position + glm::vec3(2.0f, 0.0f, -1.5f));
    auto chair1 = model_from_obj_file("assets/models/SimpleOldTownAssets/ChairCafeWhite01.obj", "Chair 1");
    chair1.set_local_transform(chair1_offset);
    scene_manager.add_model(chair1);

    glm::mat4 chair2_offset = glm::translate(glm::mat4(1.0f), bed_position + glm::vec3(-2.0f, 0.0f, -1.5f));
    auto chair2 = model_from_obj_file("assets/models/SimpleOldTownAssets/ChairCafeWhite01.obj", "Chair 2");
    chair2.set_local_transform(chair2_offset);
    scene_manager.add_model(chair2);

    glm::mat4 bookcase_offset = glm::translate(glm::mat4(1.0f), bed_position + glm::vec3(0.0f, 0.0f, -6.0f));
    auto bookcase = model_from_obj_file("assets/models/SimpleOldTownAssets/BookCase01.obj", "Bookcase");
    bookcase.set_local_transform(bookcase_offset);
    scene_manager.add_model(bookcase);
    //scene_manager.add_light(flashlight);


    // ─── Create camera ───────────────────────────────────────────────────
    Camera::CameraObj camera(1280, 720);
    camera.set_position(right_spot_light.get_position());
    camera.set_direction(right_spot_light.get_direction());

#ifdef DEBUG_DEPTH
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f};

    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
#endif

    // ─── Main loop ───────────────────────────────────────────────────────
    bool running = true;
    Uint64 lastTicks = SDL_GetPerformanceCounter();

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
            if ( camera.intersectSphereAABB(
                    camera.get_position(),
                    camera.get_radius(),
                    model->get_aabbmin(),
                    model->get_aabbmax()) )
            {
                // std::cout << "Collision with: " << model->name() << "\n";
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
        //scene_manager.set_spotlight(0, camera.get_position(), camera.get_direction());
        //scene_manager.set_spotlight(2, camera.get_position(), camera.get_direction());
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
    // ─── Cleanup ─────────────────────────────────────────────────────────
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
