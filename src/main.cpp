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

int main() {
    // ─── Initialize SDL + OpenGL ──────────────────────────────────────────
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow(
        "Simple Cube",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_GLContext glCtx = SDL_GL_CreateContext(window);

    glewInit();
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 1280, 720);

    ObjectLoader::OBJLoader cube_loader;
    cube_loader.read_from_file("assets/models/test.obj");
    //cube_loader.debug_dump();

    ObjectLoader::OBJLoader lederliege;
    lederliege.read_from_file("assets/models/lederliege.obj");
    //lederliege.debug_dump();

    ObjectLoader::OBJLoader cottage_loader;
    cottage_loader.read_from_file("assets/models/cottage_obj.obj");
    //cottage_loader.debug_dump();

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

    Model::Model couch(lederliege);
    Model::Model light_sphere(sphere_loader);


    Light flashlight(
        LightType::SPOT,
        glm::vec3(0.0f),                    // position
        glm::vec3(0.0f, 0.0f, -1.0f),       // direction
        glm::vec3(0.1f),                    // ambient
        glm::vec3(1.0f),                    // diffuse
        glm::vec3(1.0f),                    // specular
        glm::cos(glm::radians(12.5f)),      // cutoff
        glm::cos(glm::radians(17.5f)),       // outer cutoff
        1024,
        1024,
        1.0f,
        100.0f,
        10.0f
    );

    Light right_spotlight(
        LightType::SPOT,
        glm::vec3(5.0f, 1.5f, 0.0f),           // position: to the right
        glm::vec3(-1.0f, 0.0f, 0.0f),          // direction: pointing left
        glm::vec3(0.1f),
        glm::vec3(1.0f),
        glm::vec3(1.0f),
        glm::cos(glm::radians(15.0f)),         // inner cone
        glm::cos(glm::radians(25.0f)),         // outer cone
        1024, 1024,
        1.0f, 100.0f,
        10.0f
    );

    light_sphere.set_local_transform(glm::translate(glm::mat4(1.0f), right_spotlight.get_position()));

    SceneManager::SceneManager scene_manager(1280, 720);
    scene_manager.add_shader(blinnphong);
    scene_manager.add_shader(depth_2d);
    scene_manager.add_shader(depth_cube);
    scene_manager.add_model(couch);
    scene_manager.add_model(light_sphere);
    //scene_manager.add_light(flashlight);
    scene_manager.add_light(right_spotlight);


    // ─── Create camera ───────────────────────────────────────────────────
    Camera::CameraObj camera(1280, 720);


    #ifdef DEBUG_DEPTH
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
        };

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

    // ─── Main loop ───────────────────────────────────────────────────────
    bool   running   = true;
    Uint64 lastTicks = SDL_GetPerformanceCounter();
    glm::vec3 last_camera_position;
    while (running) {
        // 1) compute Δt
        Uint64 now = SDL_GetPerformanceCounter();
        float  dt  = float(now - lastTicks)
                    / float(SDL_GetPerformanceFrequency());
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
        for (auto* model : scene_manager.get_models()) {
            if ( camera.intersectSphereAABB(
                    camera.get_position(),
                    camera.get_radius(),
                    model->get_aabbmin(),
                    model->get_aabbmax()) )
            {
                // collision! revert to last safe position
                camera.set_position(last_camera_position);
                break;
            }
        }
        // 4) clear and render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 proj = camera.get_projection_matrix();
        glm::vec3 camera_position = camera.get_position();
        glm::vec3 camera_direction = camera.get_direction();  
        glm::mat4 vp   = proj * view;
        //scene_manager.set_spotlight(0, camera_position, camera_direction);
        scene_manager.render_depth_pass();
        #ifdef DEBUG_DEPTH
            depth_debug.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, flashlight.get_depth_texture());
            glUniform1i(depth_debug.get_uniform_location("depthMap"), 0);
            glUniform1f(depth_debug.get_uniform_location("near_plane"), 1.0f);
            glUniform1f(depth_debug.get_uniform_location("far_plane"), 100.0f);
            GLint depthMapLoc = depth_debug.get_uniform_location("depthMap");
            glUniform1i(depthMapLoc, 0);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        #endif 
        scene_manager.render(vp);
        //cube_model.draw(vp);
        SDL_GL_SwapWindow(window);
    }
    //─── Cleanup ─────────────────────────────────────────────────────────
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}