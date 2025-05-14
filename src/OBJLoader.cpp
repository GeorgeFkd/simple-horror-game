#include "OBJLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>



void ObjectLoader::OBJLoader::read_from_file(const std::string &filename) {
  std::cout << "Reading .obj file from: " << filename << "\n";

  std::ifstream in(filename);
  if (!in.is_open()) {
      throw std::runtime_error("Failed to open OBJ file: " + filename);
  }
  std::cout << "File is open\n";

  std::string line;

  int current_mat_id = -1;
  int current_group_id = -1;

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
      read_faceLimited(p, current_mat_id, current_group_id);
      break;
    case LineType::Mtllib:
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading MTL lib..." << std::endl;
      }
      #endif
      read_mtllib(p, filename);
      break;
    case LineType::Group:
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading Group..." << std::endl;
      }
      #endif
      add_new_group(p, current_group_id);
      break;
    case LineType::Usemtl:
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading Use MTL..." << std::endl;
      }
      #endif
      read_usemtl(p, current_mat_id);
      break;
    case LineType::Comment: 
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading Comment..." << std::endl;
      }
      #endif
      break;
    case LineType::Unknown:
      #ifdef DEBUG_OBJLOADER
      {
        std::cout<< "Reading Unknown..." << std::endl;
      }
      #endif
    default: break;
    }

  }
  
  this->load_textures();


}
static GLuint load_texture_from_file(const std::string& filepath) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return 0;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return texture_id;
}

void ObjectLoader::OBJLoader::load_textures() {
  std::cout << "Now preparing materials\n";
  for (auto& material : m_materials) {
        if (!material.map_Kd.empty()) {
            GLuint id = load_texture_from_file(material.map_Kd);
            #ifdef DEBUG_OBJLOADER
            std::cout << "Loaded map_Kd: " << material.map_Kd << " → ID " << id << std::endl;
            #endif
            material.tex_Kd = id; // if you add it
        }

        if (!material.map_Ka.empty()) {
            GLuint id = load_texture_from_file(material.map_Ka);
            #ifdef DEBUG_OBJLOADER
            std::cout << "Loaded map_Ka: " << material.map_Ka << " → ID " << id << std::endl;
            #endif
            material.tex_Ka = id;
        }

        if (!material.map_Ks.empty()) {
            GLuint id = load_texture_from_file(material.map_Ks);
            #ifdef DEBUG_OBJLOADER
            std::cout << "Loaded map_Ks: " << material.map_Ks << " → ID " << id << std::endl;
            #endif
            material.tex_Ks = id;
        }

        if (!material.map_Bump.empty()) {
            GLuint id = load_texture_from_file(material.map_Bump);
            #ifdef DEBUG_OBJLOADER
            std::cout << "Loaded map_Bump: " << material.map_Bump << " → ID " << id << std::endl;
            #endif
            material.tex_Bump = id;
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
  float tmp[2];
  if (!parse_components<2>(buff, tmp)) return;
  glm::vec2 v{ tmp[0], tmp[1]};
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

void ObjectLoader::OBJLoader::read_faceLimited(const char *buff, int current_mat_id, int current_group_id) {
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

    Face f;
    f.vertices  = vIdx;
    f.texcoords = tIdx;
    f.normals   = nIdx;

    f.material_id = current_mat_id;
    f.group_id    = current_group_id;

    #ifdef DEBUG_OBJLOADER
      std::cout << "Parsed face slot count: " << slot << "\n";
      std::cout << "  verts: " << vIdx.x<<","<<vIdx.y<<","<<vIdx.z<<","<<vIdx.w<<"\n";
      std::cout << "  tcs:   " << tIdx.x<<","<<tIdx.y<<","<<tIdx.z<<","<<tIdx.w<<"\n";
      std::cout << "  norms: " << nIdx.x<<","<<nIdx.y<<","<<nIdx.z<<","<<nIdx.w<<"\n";
    #endif

    m_faces.push_back(f);
}

void ObjectLoader::OBJLoader::debug_dump() const {
  std::cout << "=== OBJ Debug Dump ===\n";
  std::cout << "Materials (" << m_materials.size() << "):\n";
  for (size_t i = 0; i < m_materials.size(); ++i) {
    auto const& M = m_materials[i];
    std::cout << " ["<<i<<"] '"<< M.name << "'  "
              << "Ka("<< M.Ka.r<<","<<M.Ka.g<<","<<M.Ka.b<<")  "
              << "Kd("<< M.Kd.r<<","<<M.Kd.g<<","<<M.Kd.b<<")  "
              << "Ks("<< M.Ks.r<<","<<M.Ks.g<<","<<M.Ks.b<<")  "
              << "Ke("<< M.Ke.r<<","<<M.Ke.g<<","<<M.Ke.b<<")  "
              << "Ni="<< M.Ni << " "
              << "d="<< M.d << " " 
              << "illum="<< M.illum << " ";
    if (!M.map_Kd.empty())
      std::cout << "  map_Kd='"<<M.map_Kd<<"'";
    if (!M.map_Bump.empty()) 
      std::cout << "  map_Bump='" << M.map_Bump << "'";
    std::cout << "\n";
  }

  std::cout << "Groups (" << m_groups.size() << "):\n";
  for (size_t i = 0; i < m_groups.size(); ++i) {
    std::cout << " ["<<i<<"] '"<< m_groups[i] <<"'\n";
  }

  // std::cout << "Faces (" << m_faces.size() << "):\n";
  // for (size_t i = 0; i < m_faces.size(); ++i) {
  //   auto const& F = m_faces[i];
  //   std::cout << " ["<<i<<"] mat="<<F.material_id
  //             << " grp="<<F.group_id
  //             << " verts=("
  //                <<F.vertices.x<<","<<F.vertices.y<<","<<F.vertices.z<<","<<F.vertices.w<<")\n";
  // }
  std::cout << "======================\n\n";
}


void ObjectLoader::OBJLoader::read_usemtl(const char *buff, int &current_mat_id) {
  const char* p = buff;
  while (*p && std::isspace(*p)) ++p;

  const char* q = p;
  while (*q && !std::isspace(*q) && *q != '#') ++q;

  std::string name{ p, size_t(q - p) };
  auto it = m_mat_name_to_id.find(name);
  if (it != m_mat_name_to_id.end()) {
    current_mat_id = it->second;
  } else {
    current_mat_id = -1;
  }
}

void ObjectLoader::OBJLoader::read_mtllib(const char* buff, const std::string& obj_filename) {
  const char* p = buff;
  while (*p && std::isspace(*p)) ++p;
  const char* q = p;
  while (*q && !std::isspace(*q) && *q != '#') ++q;
  std::string mtl_name{ p, size_t(q - p) };

  auto slash = obj_filename.find_last_of("/\\");
  std::string dir  = (slash == std::string::npos ? "" : obj_filename.substr(0, slash+1));
  std::string path = dir + mtl_name;

  std::ifstream file{path, std::ios::binary};
  if (!file) {
      std::cerr << "Failed to open MTL: " << path << "\n";
      return;
  }
  file.seekg(0, std::ios::end);
  size_t      sz = file.tellg();
  std::string contents(sz, '\0');
  file.seekg(0);
  file.read(contents.data(), sz);

  Material current_mat;
  auto commit_mat = [&]() {
      if (!current_mat.name.empty()) {
          int id = m_materials.size();
          m_mat_name_to_id[current_mat.name] = id;
          m_materials.push_back(std::move(current_mat));
          current_mat = Material{};
      }
  };

  std::string_view view{contents};
  size_t           pos = 0;
  while (pos < view.size()) {
      size_t eol = view.find_first_of("\r\n", pos);
      auto   line = view.substr(pos,
                        eol == std::string_view::npos
                          ? view.size()-pos
                          : eol - pos);
      pos = (eol == std::string_view::npos
              ? view.size()
              : view.find_first_not_of("\r\n", eol));

      size_t i = 0;
      while (i < line.size() && std::isspace(line[i])) ++i;
      if (i >= line.size() || line[i] == '#') continue;

      size_t key_end = i;
      while (key_end < line.size() && !std::isspace(line[key_end])) ++key_end;
      std::string_view key  = line.substr(i, key_end - i);
      size_t           dpos = line.find_first_not_of(" \t", key_end);
      std::string_view data = dpos == std::string_view::npos
                              ? std::string_view{}
                              : line.substr(dpos);

      if (key == "newmtl") {
          commit_mat();
          current_mat.name = std::string{data};
      }
      else if (key == "Ka") {
          float tmp[3];
          if (parse_components_sv<3>(data, tmp))
              current_mat.Ka = { tmp[0], tmp[1], tmp[2] };
      }
      else if (key == "Kd") {
          float tmp[3];
          if (parse_components_sv<3>(data, tmp))
              current_mat.Kd = { tmp[0], tmp[1], tmp[2] };
      }
      else if (key == "Ks") {
          float tmp[3];
          if (parse_components_sv<3>(data, tmp))
              current_mat.Ks = { tmp[0], tmp[1], tmp[2] };
      }
      else if (key == "Ns") {
          float tmp[1];
          if (parse_components_sv<1>(data, tmp))
              current_mat.Ns = tmp[0];
      }
      else if (key == "Ke") {
        float tmp[3];
        if (parse_components_sv<3>(data, tmp))
            current_mat.Ke = { tmp[0], tmp[1], tmp[2] };
      }
      else if (key == "Ni") {
          float tmp[1];
          if (parse_components_sv<1>(data, tmp))
              current_mat.Ni = tmp[0];
      }
      else if (key == "d") {
          float tmp[1];
          if (parse_components_sv<1>(data, tmp))
              current_mat.d = tmp[0];
      }
      else if (key == "illum") {
          int ival = 0;
          auto [_, ec] = std::from_chars(data.data(), data.data() + data.size(), ival);
          if (ec == std::errc())
              current_mat.illum = ival;
      }
      else if (key == "map_Ka") {
          current_mat.map_Ka = dir + std::string{data};
      }
      else if (key == "map_Kd") {
          current_mat.map_Kd = dir + std::string{data};
      }
      else if (key == "map_Ks") {
          current_mat.map_Ks = dir + std::string{data};
      }
      else if (key == "bump" || key == "map_bump") {
          current_mat.map_Bump = dir + std::string{data};
    }
  }

  commit_mat();
}

void ObjectLoader::OBJLoader::add_new_group(const char *buff, int &current_group_id) {
  const char* p = buff;
  while (*p && std::isspace(*p)) ++p;

  const char* q = p;
  while (*q && !std::isspace(*q) && *q != '#') ++q;

  std::string name{ p, size_t(q - p) };
  auto it = m_group_name_to_id.find(name);
  if (it == m_group_name_to_id.end()) {
    int id = m_groups.size();
    m_groups.push_back(name);
    m_group_name_to_id[name] = id;
    current_group_id = id;
  } else {
    current_group_id = it->second;
  }
}
