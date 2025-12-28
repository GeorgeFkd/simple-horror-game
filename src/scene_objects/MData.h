#pragma once
#include "EntityID.h"
#include "SubMesh.h"
#include "Vertex.h"
#include <glm/vec3.hpp>

class MData {
  public:
    MData();
    MData(std::vector<GLuint>&& indices, std::vector<Vertex>&& unique_vertices,
          std::vector<SubMesh>&& submeshes);
    ~MData();
    std::vector<GLuint>  indices;
    std::vector<Vertex>  unique_vertices;
    std::vector<SubMesh> submeshes;
    // Object-space AABB (min/max corners in mesh local coords)
    glm::vec3 localaabbmin, localaabbmax;
    EntityID  id;
    void      initialize();

  private:
    void                                                      initialize_local_aabb();
    std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> prepare_bitangents();
    void orthogonalize_and_normalize_tb(Vertex&                       vertex,
                                        const std::vector<glm::vec3>& accumulated_tangent,
                                        const std::vector<glm::vec3>& accumulated_bitangent,
                                        const size_t                  index);
};
