#pragma once
#include "SubMesh.h"
#include "Vertex.h"
#include <glm/vec3.hpp>
#include "EntityID.h"


class MData {
  public:
    MData();
    ~MData();
    std::vector<GLuint>  indices;
    std::vector<Vertex>  unique_vertices;
    std::vector<SubMesh> submeshes;
    // Object-space AABB (min/max corners in mesh local coords)
    glm::vec3 localaabbmin, localaabbmax;
    EntityID id;
};
