#include "OBJLoader.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

bool starts_with(const std::string &st, const std::string &prefix) {
  return st.rfind(prefix, 0) == 0;
}

OBJLoader::OBJLoader() {}
OBJLoader::~OBJLoader() {}

void print_glmvec4(glm::vec4 v) {
  std::cout << v.x << "," << v.y << "," << v.z << "," << v.w << "\n";
}

void print_glmvec3(glm::vec3 v) {
  std::cout << v.x << "," << v.y << "," << v.z << "\n";
}
void print_glmvec2(glm::vec2 v) { std::cout << v.x << "," << v.y << "\n"; }
void OBJLoader::read_vertex(const char *buff) {
  glm::vec4 v;
  int read = sscanf(buff, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
  if (read == 3) {
    v.w = 0;
  }
  print_glmvec4(v);
  m_vertices.push_back(v);
}

void OBJLoader::read_texcoord(const char *buff) {
  // u,[v,w] coords, v,w optional, default 0
  glm::vec3 v;
  int read = sscanf(buff, "%f %f %f", &v.x, &v.y, &v.z);
  print_glmvec2(v);
  if (read == 1) {
    v.y = 0;
    v.z = 0;
  } else if (read == 2) {
    v.z = 0;
  }
  m_texture_coords.push_back(v);
}
void OBJLoader::read_normal(const char *buff) {
  glm::vec3 v;
  sscanf(buff, "%f %f %f", &v.x, &v.y, &v.z);
  print_glmvec3(v);
  m_vertex_normals.push_back(v);
}
void OBJLoader::read_faceLimited(const char *buff) {
  // v/vt/vn or v//vn or v/vt or v
  // 4 components per face are supported for now
  unsigned int MAX_COMPONENTS_PER_FACE = 4;
  glm::ivec3 face_components[MAX_COMPONENTS_PER_FACE];
  int offset = 0;
  int read_bytes;
  for (int i = 0; i < MAX_COMPONENTS_PER_FACE; i++) {
    // TODO chop into triangles
    if (sscanf(buff + offset, "%d/%d/%d%n", &(face_components[i].x), &(face_components[i].y),
               &(face_components[i].z),&read_bytes) >= 3) {
      // v/vt/vn
      std::cout << "v/vt/vn format\n";
      
    offset += read_bytes;
      continue;
    }
    if (sscanf(buff + offset, "%d//%d%n", &(face_components[i].x), &(face_components[i].y),&read_bytes) >=
        2) {
      // v//vn
      face_components[i].z = -1;
      std::cout << "v//vn format\n";
    offset += read_bytes;
      continue;
    }
    if (sscanf(buff + offset, "%d/%d%n", &(face_components[i]).x, &(face_components[i].z),&read_bytes) >=
        2) {

      // v/vt
      
      face_components[i].y = -1;
      std::cout << "v/vt format\n";
    offset += read_bytes;
      continue;
    }

    if (sscanf(buff,"%d%n",&(face_components[i].x),&read_bytes) >= 1) {
      face_components[i].y = -1;
      face_components[i].z = -1;
      std::cout << "v format\n";
    offset += read_bytes;
      continue;
    }


  }
  Face f;
  f.normals = glm::ivec4(face_components[0].y, face_components[1].y,
                         face_components[2].y, face_components[3].y);
  f.vertices = glm::ivec4(face_components[0].x, face_components[1].x,
                          face_components[2].x, face_components[3].x);
  f.texcoords = glm::ivec4(face_components[0].z, face_components[1].z,
                           face_components[2].z, face_components[3].z);
  std::cout << "Face(Vertices,Normals,TextureCoords)\n";
  print_glmvec3(f.vertices);
  print_glmvec3(f.normals);
  print_glmvec3(f.texcoords);

  m_faces.push_back(f);
}
void OBJLoader::read_usemtl(const char *buff, int &materialID) {}
void OBJLoader::read_mtllib(const char *buff, const std::string &filename) {}
void OBJLoader::add_new_group(const char *buff, int &materialID) {}

void OBJLoader::read_from_file(const std::string &filename) {
  std::cout << "Reading .obj file from: " << filename << "\n";

  FILE *objFile = fopen(filename.c_str(), "r");
  std::cout << "File is open\n";
  const int BUFFER_SIZE = 1024;
  char buff[BUFFER_SIZE];
  int str_pos;
  char str[BUFFER_SIZE];
  int currentMaterialID = 0;
  while (fgets(buff, BUFFER_SIZE, objFile) != NULL) {
    if (sscanf(buff, "%s %n", str, &str_pos) >= 1) {
      if (strcmp(str, "v") == 0)
        read_vertex(buff + str_pos);
      else if (strcmp(str, "vt") == 0)
        read_texcoord(buff + str_pos);
      else if (strcmp(str, "vn") == 0)
        read_normal(buff + str_pos);
      else if (strcmp(str, "f") == 0)
        read_faceLimited(buff + str_pos);
      // else if (strcmp(str, "usemtl") == 0)
      //   read_usemtl(buff + str_pos, currentMaterialID);
      // else if (strcmp(str, "mtllib") == 0)
      //   read_mtllib(buff + str_pos, filename);
      // else if (strcmp(str, "g") == 0 || strcmp(str, "o") == 0)
      //   add_new_group(buff + str_pos, currentMaterialID);
      else if (strcmp(str, "#") == 0) { /* ignoring this line */
      } else {                          /* ignoring this line */
      }
    }
  }

  return;
}
