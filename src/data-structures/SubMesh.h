#pragma once
#include <GL/glew.h>
#include <vector>
#include "Material.h"

struct SubMesh {
  Material mat;
  SubMesh() {};
  SubMesh(GLuint offset, GLuint count, const Material& material)
        : index_offset(offset), index_count(count), mat(material) {}
  std::vector<GLuint> indices;
  GLuint index_offset;   // offset into the big EBO
  GLuint index_count;
};
