#include "EventLoop.h"

void EventLoop::pollEvents(float dt){
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        for (auto& evHandler : registeredHandlers) {
            evHandler(&ev,dt);
        }
    }
}

void EventLoop::addEventHandler(std::function<void(SDL_Event*,float)> handler) {
    registeredHandlers.emplace_back(handler);
}
