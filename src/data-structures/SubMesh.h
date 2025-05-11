#pragma once
#include <GL/glew.h>
#include <vector>
#include "Material.h"

struct SubMesh {
  Material mat;
  std::vector<GLuint> indices;
  GLuint index_offset;   // offset into the big EBO
  GLuint index_count;
};