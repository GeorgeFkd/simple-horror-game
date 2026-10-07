#pragma once
// Values that differ between desktop OpenGL 3.3 and WebGL2 (OpenGL ES 3.0, used by the
// Emscripten build). Desktop values are what the renderer always used.
#include <GL/glew.h>

#ifdef __EMSCRIPTEN__
// WebGL2 requires sized internal formats for depth and single channel textures.
constexpr GLint DEPTH_TEXTURE_INTERNAL_FORMAT  = GL_DEPTH_COMPONENT32F;
constexpr GLint SINGLE_CHANNEL_INTERNAL_FORMAT = GL_R8;
// GL_CLAMP_TO_BORDER / GL_TEXTURE_BORDER_COLOR do not exist in WebGL2.
constexpr GLint SHADOW_MAP_WRAP_MODE = GL_CLAMP_TO_EDGE;
constexpr bool  SUPPORTS_TEXTURE_BORDER_COLOR = false;
// WebGL2 has no geometry shaders, point light cube maps are rendered one face at a time.
constexpr bool SUPPORTS_GEOMETRY_SHADERS = false;
#else
constexpr GLint DEPTH_TEXTURE_INTERNAL_FORMAT  = GL_DEPTH_COMPONENT;
constexpr GLint SINGLE_CHANNEL_INTERNAL_FORMAT = GL_RED;
constexpr GLint SHADOW_MAP_WRAP_MODE           = GL_CLAMP_TO_BORDER;
constexpr bool  SUPPORTS_TEXTURE_BORDER_COLOR  = true;
constexpr bool  SUPPORTS_GEOMETRY_SHADERS      = true;
#endif

// Largest shadow map side the renderer allocates. The browser build targets integrated GPUs and
// redraws every shadow map each frame, so it caps them to keep the frame rate above 30 FPS.
#ifdef __EMSCRIPTEN__
constexpr int MAX_SHADOW_MAP_SIZE = WEB_MAX_SHADOW_MAP_SIZE;
#else
constexpr int MAX_SHADOW_MAP_SIZE = 1 << 30; // unlimited
#endif

struct ShadowMapExtent {
    int width;
    int height;
};

// Scales (width, height) down to fit MAX_SHADOW_MAP_SIZE, keeping the aspect ratio so the light's
// projection matrix stays valid.
inline ShadowMapExtent shadow_map_extent(int width, int height) {
    int largest = width > height ? width : height;
    if (largest <= MAX_SHADOW_MAP_SIZE) {
        return {width, height};
    }
    return {int((long long)width * MAX_SHADOW_MAP_SIZE / largest),
            int((long long)height * MAX_SHADOW_MAP_SIZE / largest)};
}
