#include "OBJLoader.h"


void ObjectLoader::OBJLoader::read_from_file(const std::string &filename) {
  std::cout << "Reading .obj file from: " << filename << "\n";

  std::ifstream in(filename);
  if (!in.is_open()) {
      throw std::runtime_error("Failed to open OBJ file: " + filename);
  }
  std::cout << "File is open\n";

  std::string line;
  int current_material_id = 0;

  while(std::getline(in, line)){

    if (line.empty()) continue;

    // Convert line to c pointer
    const char *p = line.c_str();
    // skip any whitespaces
    while (std::isspace(*p)) ++p;
    //Mark the start of a keyword
    const char *kw_start = p;
    //find the end of keyword
    while(*p && !std::isspace(*p)) ++p;
    std::string_view keyword{kw_start, size_t(p - kw_start)};
    // skip any spaces to get to data
    while (*p && std::isspace(*p)) ++p;
    switch (ObjectLoader::classify_line_type(keyword))
    {
    case LineType::Vertex:   
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading Vertex..." << std::endl;
      }
      #endif
      read_vertex(p);    
      break;
    case LineType::Texcoord: 
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading TexCoord..." << std::endl;
      }
      #endif
      read_texcoord(p);  
      break;
    case LineType::Normal:   
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading Normal..." << std::endl;
      }
      #endif
      read_normal(p);    
      break;
    case LineType::Face:     
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading Face..." << std::endl;
      }
      #endif
      read_faceLimited(p); 
      break;
    default: break;
    }

  }
}

void ObjectLoader::OBJLoader::read_normal(const char* buff) {
  float tmp[3];
  if (!parse_components<3>(buff, tmp)) return;
  glm::vec3 v{ tmp[0], tmp[1], tmp[2] };
  #ifdef DEBUG_OBJLOADER
    print_glmvec3(v);
  #endif
  m_vertex_normals.push_back(v);
}

void ObjectLoader::OBJLoader::read_texcoord(const char* buff) {
  float tmp[3];
  if (!parse_components<3>(buff, tmp)) return;
  glm::vec3 v{ tmp[0], tmp[1], tmp[2] };
  #ifdef DEBUG_OBJLOADER
    print_glmvec2(glm::vec2{v.x, v.y});
  #endif
  m_texture_coords.push_back(v);
}

void ObjectLoader::OBJLoader::read_vertex(const char* buff) {
  float tmp[4];
  if (!parse_components<4>(buff, tmp)) return;
  glm::vec4 v{ tmp[0], tmp[1], tmp[2], tmp[3] };
  #ifdef DEBUG_OBJLOADER
    print_glmvec4(v);
  #endif
  m_vertices.push_back(v);
}

void ObjectLoader::OBJLoader::read_faceLimited(const char *buff) {
    // Prepare arrays, defaulting everything to -1
    glm::ivec4 vIdx(-1), tIdx(-1), nIdx(-1);

    const char* p     = buff;
    const char* end   = buff + std::strlen(buff);
    int slot          = 0;

    // Parse up to 4 vertex/texcoord/normal groups
    while (slot < 4 && p < end && !std::isspace(*p)) {
        // 1) parse vertex index
        int vi;
        auto [p1, ec1] = std::from_chars(p, end, vi);
        if (ec1 != std::errc()) break;
        vIdx[slot] = vi - 1;  // OBJ is 1-based

        p = p1;
        // 2) if there's a slash, maybe parse vt and/or vn
        if (p < end && *p == '/') {
            ++p;
            // if next also '/', then it's v//vn
            if (p < end && *p == '/') {
                ++p;
                int ni;
                auto [p2, ec2] = std::from_chars(p, end, ni);
                if (ec2 == std::errc()) nIdx[slot] = ni - 1;
                p = p2;
            } else {
                // v/vt[/vn]
                int ti;
                auto [p2, ec2] = std::from_chars(p, end, ti);
                if (ec2 == std::errc()) tIdx[slot] = ti - 1;
                p = p2;
                // now maybe "/vn"
                if (p < end && *p == '/') {
                    ++p;
                    int ni;
                    auto [p3, ec3] = std::from_chars(p, end, ni);
                    if (ec3 == std::errc()) nIdx[slot] = ni - 1;
                    p = p3;
                }
            }
        }

        // 3) skip any whitespace before next
        while (p < end && std::isspace(*p)) ++p;
        ++slot;
    }

    // Store into your Face struct
    Face f;
    f.vertices  = vIdx;
    f.texcoords = tIdx;
    f.normals   = nIdx;

    #ifdef DEBUG_OBJLOADER
      std::cout << "Parsed face slot count: " << slot << "\n";
      std::cout << "  verts: " << vIdx.x<<","<<vIdx.y<<","<<vIdx.z<<","<<vIdx.w<<"\n";
      std::cout << "  tcs:   " << tIdx.x<<","<<tIdx.y<<","<<tIdx.z<<","<<tIdx.w<<"\n";
      std::cout << "  norms: " << nIdx.x<<","<<nIdx.y<<","<<nIdx.z<<","<<nIdx.w<<"\n";
    #endif

    m_faces.push_back(f);
}

void ObjectLoader::OBJLoader::read_usemtl(const char *buff, int &material_id) {}
void ObjectLoader::OBJLoader::read_mtllib(const char *buff, const std::string &filename) {}
void ObjectLoader::OBJLoader::add_new_group(const char *buff, int &material_id) {}
