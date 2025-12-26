#pragma once
#include "SubMesh.h"
#include "Vertex.h"
#include <glm/vec3.hpp>


class MData {
  public:
    ~MData();
    std::vector<GLuint>  indices;
    std::vector<Vertex>  unique_vertices;
    std::vector<SubMesh> submeshes;
    // Object-space AABB (min/max corners in mesh local coords)
    glm::vec3 localaabbmin, localaabbmax;
    // for the same model data the memory reserved will be the same, thus fewer allocations
    // the problem is that the opengl buffers should be a separate struct
    GLuint vao, vbo, ebo = 0;
    void reserve_open_gl_memory();
};
