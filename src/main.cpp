// main.cpp
#include <SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OBJLoader.h"
#include "Renderer.h"

int main(int /*argc*/, char** /*argv*/){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow(
        "Simple Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);

    glewInit();
    glEnable(GL_DEPTH_TEST);
    glViewport(0,0,1280,720);

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    ObjectLoader::OBJLoader loader;
    loader.read_from_file("assets/models/test.obj");
    loader.debug_dump();

    Renderer::RendererObj renderer(1280, 720);
    renderer.load_model(loader);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0,0,3),   // eye
        glm::vec3(0,0,0),   // center
        glm::vec3(0,1,0));  // up

    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        1280.0f / 720.0f,
        0.1f,
        100.0f);

    bool running = true;
    while(running){
      SDL_Event ev;
      while(SDL_PollEvent(&ev)){
        if(ev.type==SDL_QUIT) running = false;

        if(ev.type==SDL_WINDOWEVENT &&
           ev.window.event==SDL_WINDOWEVENT_SIZE_CHANGED)
        {
          int w = ev.window.data1,
              h = ev.window.data2;
          glViewport(0,0,w,h);
          proj = glm::perspective(
            glm::radians(45.0f),
            float(w)/float(h),
            0.1f, 100.0f);
        }
      }

      glClearColor(0.1f,0.1f,0.1f,1.0f);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

      glm::mat4 vp = proj * view;
      renderer.render(vp);

      SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
