#include <SDL2/SDL.h>
#include <functional>
class EventLoop {
public:
    void pollEvents(float dt);
    void addEventHandler(std::function<void(SDL_Event*,float)>);
private:
    std::vector<std::function<void(SDL_Event*,float)>> registeredHandlers;
};
