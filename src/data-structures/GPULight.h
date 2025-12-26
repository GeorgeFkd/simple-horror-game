#pragma once
#include "Light.h"
#include <GL/glew.h>
class GPULight {
public:
    GPULight();
    ~GPULight();
    void reserve_opengl_memory(Light* light);
    GLuint   depth_map_fbo;
    GLuint   depth_map;
};
