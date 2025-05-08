// main.cpp
#include <SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OBJLoader.h"
#include "Renderer.h"
#include "Camera.h"

int main() {
    // ——— Initialize SDL + OpenGL ——————————————————————————————————
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

    // ——— Load your model + renderer ———————————————————————————————
    ObjectLoader::OBJLoader loader;
    loader.read_from_file("assets/models/test.obj");
    loader.debug_dump();

    Renderer::RendererObj renderer(1280, 720);
    renderer.load_model(loader);

    // ——— Create the camera ——————————————————————————————————————
    Camera::CameraObj camera(1280, 720);

    // ——— Main loop —————————————————————————————————————————————
    bool   running   = true;
    Uint64 lastTicks = SDL_GetPerformanceCounter();

    while (running) {
        // — compute delta time —
        Uint64 now = SDL_GetPerformanceCounter();
        float  dt  = float(now - lastTicks)
                    / float(SDL_GetPerformanceFrequency());
        lastTicks = now;

        // — pump events —
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }
            // feed all events to the camera
            camera.process_input(ev);

            // update viewport on resize (camera.process_input already updates aspect)
            if (ev.type == SDL_WINDOWEVENT &&
                ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                int w = ev.window.data1,
                    h = ev.window.data2;
                glViewport(0, 0, w, h);
            }
        }

        // — update camera movement from keyboard —
        camera.update(dt);

        // — clear screen —
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // — build view/projection —
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix();
        glm::mat4 vp   = proj * view;

        // — render —
        renderer.render(vp);

        SDL_GL_SwapWindow(window);
    }

    // ——— Cleanup ————————————————————————————————————————————————
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
