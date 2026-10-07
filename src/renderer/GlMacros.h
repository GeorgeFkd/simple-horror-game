#ifndef GL_MACROS_H
#define GL_MACROS_H
#include <GL/glew.h>
#include <iostream>
#define ASSERT(x) assert(x)
#if defined(__EMSCRIPTEN__) && defined(NDEBUG)
// In the browser glGetError() is a synchronous round-trip to the WebGL process, calling it around
// every GL call costs a lot of FPS. Release web builds skip the error checks entirely.
#define GLCall(x) x
#else
#define GLCall(x)                                                                                  \
    GLClearError();                                                                                \
    x;                                                                                             \
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#endif

namespace GlHelpers {

static void GLClearError() {
    while (glGetError() != GL_NO_ERROR)
        ;
}

static bool GLLogCall(const char* function, const char* file, int line) {

    while (GLenum error = glGetError()) {

        std::cout << "[OpenGL Error] (" << error << ")" << "at function: " << function
                  << ", file: " << file << ", line: " << line << "\n";
        return false;
    }
    return true;
}
} // namespace GlHelpers
#endif
