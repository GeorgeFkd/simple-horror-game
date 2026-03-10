#include <iostream>

#include <SDL2/SDL.h>
namespace Logging {

#define PERF(label,block) \
    do { \
        Uint64 _start = SDL_GetPerformanceCounter(); \
        block; \
        Uint64 _end = SDL_GetPerformanceCounter(); \
        float elapsedMS = (_end - _start) / (float)SDL_GetPerformanceFrequency() * 1000.0f; \
        std::cout << label << " TIME: " << elapsedMS << "ms. \n"; \
    } while (0)

} // namespace Logging
