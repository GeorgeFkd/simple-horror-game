#pragma once
#include "MData.h"

class GPUMesh {
public:
    GPUMesh();
    ~GPUMesh();
    GLuint vao, vbo, ebo = 0;
    void reserve_opengl_memory(MData* model_data);
};
