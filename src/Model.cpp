#include "Model.h"

void Model::Model::debug_dump() const {
    size_t total_indices   = 0;
    size_t total_triangles = 0;
    for (auto const& sm : submeshes) {
        total_indices   += sm.index_count;
        total_triangles += sm.index_count / 3;
    }

    std::cout 
      << "  >>> Model built: "
      << unique_vertices.size() << " unique vertices, "
      << total_triangles         << " triangles, "
      << total_indices           << " indices total\n"
      << "      Submeshes: "     << submeshes.size() << "\n"
      << "      AABB local min = ("
         << localaabbmin.x << ", "
         << localaabbmin.y << ", "
         << localaabbmin.z << ")\n"
      << "      AABB local max = ("
         << localaabbmax.x << ", "
         << localaabbmax.y << ", "
         << localaabbmax.z << ")" << std::endl;
}

Model::Model::Model(const ObjectLoader::OBJLoader& loader,const std::string& label)
  : local_transform(1.0f)
  , world_transform(1.0f)
  , localaabbmin(std::numeric_limits<float>::max())
  , localaabbmax(-std::numeric_limits<float>::max())
, label(label)
{
    // build unique_vertices & a cache
    std::unordered_map<Vertex, GLuint, VertexHasher> cache;
    cache.reserve(loader.m_faces.size() * 4);

    // bucket indices by material_id
    std::unordered_map<int, std::vector<GLuint>> buckets;

    auto add_vertex = [&](int vi, int ti, int ni) {
        Vertex vert;
        vert.position = glm::vec3(loader.m_vertices[vi]);
        if (ti >= 0 && ti < (int)loader.m_texture_coords.size()) {
            vert.texcoord = loader.m_texture_coords[ti];
        } else {
            vert.texcoord = glm::vec2{0.0f, 0.0f};
        }

        if (ni >= 0 && ni < (int)loader.m_vertex_normals.size()) {
            vert.normal = loader.m_vertex_normals[ni];
        } else {
            vert.normal = glm::vec3{0.0f, 0.0f, 1.0f};
        }

        auto [it, inserted] = cache.emplace(vert, (GLuint)unique_vertices.size());
        if (inserted) {
          unique_vertices.push_back(vert);
        }
        return it->second;
    };

    for (auto const& face : loader.m_faces) {
        int material_id = face.material_id;
        // unpack up to 4 verts; 3 if w == -1
        int vertex_count = (face.vertices.w == -1 ? 3 : 4);
        // first tri
        buckets[material_id].push_back(
            add_vertex(face.vertices[0], face.texcoords[0], face.normals[0]));
        buckets[material_id].push_back(
            add_vertex(face.vertices[1], face.texcoords[1], face.normals[1]));
        buckets[material_id].push_back(
            add_vertex(face.vertices[2], face.texcoords[2], face.normals[2]));
        // second tri if quad
        if (vertex_count == 4) {
            buckets[material_id].push_back(
                add_vertex(face.vertices[0], face.texcoords[0], face.normals[0]));
            buckets[material_id].push_back(
                add_vertex(face.vertices[2], face.texcoords[2], face.normals[2]));
            buckets[material_id].push_back(
                add_vertex(face.vertices[3], face.texcoords[3], face.normals[3]));
        }
    }

    // flatten buckets → one big index array, record submeshes
    std::vector<GLuint> all_indices;
    all_indices.reserve(
      std::accumulate(buckets.begin(), buckets.end(), 0u,
                      [](auto sum, auto &p){ return sum + p.second.size(); })
    );

    for (auto & [material_id, indexes] : buckets) {
        SubMesh sm;
        if (material_id>= 0) {
          sm.mat = loader.m_materials[material_id];  // assumes same Material layout
        } else {
          sm.mat = Material{};
        }
        sm.index_offset = (GLuint)all_indices.size();
        sm.index_count  = (GLuint)indexes.size();

        all_indices.insert(all_indices.end(), indexes.begin(), indexes.end());
        submeshes.push_back(sm);
    }

    //create & upload VAO/VBO/EBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 unique_vertices.size() * sizeof(Vertex),
                 unique_vertices.data(),
                 GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 all_indices.size() * sizeof(GLuint),
                 all_indices.data(),
                 GL_STATIC_DRAW);

    // attributes (pos, tex, norm) …
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);

    // compute local AABB
    for (auto const& v : unique_vertices) {
        localaabbmin = glm::min(localaabbmin, v.position);
        localaabbmax = glm::max(localaabbmax, v.position);
    }
}

Model::Model::~Model(){
    // Tear down GL objects in reverse order of creation:
    if (ebo) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}

void Model::Model::add_child(Model* child){
    children.push_back(child);
}

void Model::Model::set_local_transform(const glm::mat4& local_transform){
    this->local_transform = local_transform;
}

void Model::Model::update_world_transform(const glm::mat4& parent_transform) {
    world_transform =  parent_transform * local_transform;

    compute_aabb();
    for (Model* child : children) {
        child->update_world_transform(world_transform);
    }
}

void Model::Model::draw(const glm::mat4& view_projection, Shader* shader) const{
    shader->use();

    // upload matrices
    GLint loc_vp = shader->get_uniform_location("uViewProj");
    GLint loc_m  = shader->get_uniform_location("uModel");
    glUniformMatrix4fv(loc_vp, 1, GL_FALSE, glm::value_ptr(view_projection));
    glUniformMatrix4fv(loc_m,  1, GL_FALSE, glm::value_ptr(world_transform));

    // pre-lookup all material uniforms once
    GLint loc_ka      = shader->get_uniform_location("material.ambient");
    GLint loc_kd      = shader->get_uniform_location("material.diffuse");
    GLint loc_ks      = shader->get_uniform_location("material.specular");
    GLint loc_ke      = shader->get_uniform_location("material.emissive");
    GLint loc_ns      = shader->get_uniform_location("material.shininess");
    GLint loc_opacity = shader->get_uniform_location("material.opacity");
    GLint loc_illum   = shader->get_uniform_location("material.illumModel");
    GLint loc_ior     = shader->get_uniform_location("material.ior");
    GLint loc_useTex   = shader->get_uniform_location("useTexture");
    GLint loc_diffuse  = shader->get_uniform_location("diffuseMap");
    glBindVertexArray(vao);
    for (auto const& sm : submeshes) {
      if (loc_ka      >= 0) glUniform3fv(loc_ka,      1, glm::value_ptr(sm.mat.Ka));
      if (loc_kd      >= 0) glUniform3fv(loc_kd,      1, glm::value_ptr(sm.mat.Kd));
      if (loc_ks      >= 0) glUniform3fv(loc_ks,      1, glm::value_ptr(sm.mat.Ks));
      if (loc_ke      >= 0) glUniform3fv(loc_ke,      1, glm::value_ptr(sm.mat.Ke));
      if (loc_ns      >= 0) glUniform1f (loc_ns,       sm.mat.Ns);
      if (loc_opacity >= 0) glUniform1f (loc_opacity,  sm.mat.d);
      if (loc_illum   >= 0) glUniform1i (loc_illum,    sm.mat.illum);
      if (loc_ior     >= 0) glUniform1f (loc_ior,      sm.mat.Ni);
      // Texture binding
      if (sm.mat.tex_Kd != 0 && loc_diffuse >= 0 && loc_useTex >= 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sm.mat.tex_Kd);
        glUniform1i(loc_diffuse, 0);
        glUniform1i(loc_useTex, 1);
      } else if (loc_useTex >= 0) {
        glUniform1i(loc_useTex, 0);
      }

      void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
      glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offsetPtr);
    }
    glBindVertexArray(0);
}

void Model::Model::draw_depth(Shader* shader) const {

    GLint loc_model = shader->get_uniform_location("uModel");
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, glm::value_ptr(world_transform));

    glBindVertexArray(vao);
    for (auto const& sm : submeshes) {
        void* offset_ptr = (void*)(sm.index_offset * sizeof(GLuint));
        glDrawElements(
            GL_TRIANGLES,
            sm.index_count,
            GL_UNSIGNED_INT,
            offset_ptr
        );
    }
    glBindVertexArray(0);
}

void Model::Model::compute_aabb() {
    // build 8 corners from the **object-space** box
    glm::vec3 corners[8] = {
        {localaabbmin.x,localaabbmin.y,localaabbmin.z},
        {localaabbmax.x,localaabbmin.y,localaabbmin.z},
        {localaabbmin.x,localaabbmax.y,localaabbmin.z},
        {localaabbmax.x,localaabbmax.y,localaabbmin.z},
        {localaabbmin.x,localaabbmin.y,localaabbmax.z},
        {localaabbmax.x,localaabbmin.y,localaabbmax.z},
        {localaabbmin.x,localaabbmax.y,localaabbmax.z},
        {localaabbmax.x,localaabbmax.y,localaabbmax.z}
    };

    glm::vec3 world_min( FLT_MAX ), world_max( -FLT_MAX );
    for (int i = 0; i < 8; ++i) {
        glm::vec4 wc = world_transform * glm::vec4(corners[i], 1.0f);
        glm::vec3 w  = glm::vec3(wc);
        world_min = glm::min(world_min, w);
        world_max = glm::max(world_max, w);
    }

    // store into the world‐space fields
    aabbmin = world_min;
    aabbmax = world_max;
}

