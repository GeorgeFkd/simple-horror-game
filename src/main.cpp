// main.cpp
#include <SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OBJLoader.h"
#include "SceneManager.h"
#include "Camera.h"

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

    // ─── Load model & set up renderer ────────────────────────────────────
    ObjectLoader::OBJLoader cube_loader;
    cube_loader.read_from_file("assets/models/test.obj");
    cube_loader.debug_dump();


    SceneManager::SceneManager scene_manager(1280, 720);

    Model::Model cube_model(cube_loader);
    cube_model.set_shader_program(scene_manager.get_shader_program());
    cube_model.debug_dump();

    scene_manager.add_model(cube_model);

    // ─── Create camera ───────────────────────────────────────────────────
    Camera::CameraObj camera(1280, 720);


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
            // skip non-collidable models if you tag them
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
        glm::mat4 vp   = proj * view;
        scene_manager.render(vp);
        //cube_model.draw(vp);
        SDL_GL_SwapWindow(window);
    }
    // ─── Cleanup ─────────────────────────────────────────────────────────
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}