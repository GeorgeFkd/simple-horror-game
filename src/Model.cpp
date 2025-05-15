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


Model::Model::Model(const std::vector<glm::vec3>& positions,
                    const std::vector<glm::vec3>& normals,
                    const std::vector<glm::vec2>& texcoords,
                    const std::vector<GLuint>& indices,
                    const Material& mat)
    : local_transform(1.0f),
      world_transform(1.0f),
      localaabbmin(std::numeric_limits<float>::max()),
      localaabbmax(-std::numeric_limits<float>::lowest())
{
    // Build unique vertex array
    for (size_t i = 0; i < positions.size(); ++i) {
        Vertex vert;
        vert.position = positions[i];
        vert.normal   = (i < normals.size())   ? normals[i]   : glm::vec3(0, 1, 0);
        vert.texcoord = (i < texcoords.size()) ? texcoords[i] : glm::vec2(0, 0);
        unique_vertices.push_back(vert);

        // Update local AABB
        localaabbmin = glm::min(localaabbmin, vert.position);
        localaabbmax = glm::max(localaabbmax, vert.position);
    }

    // Single submesh
    SubMesh sm;
    sm.mat = mat;
    sm.index_offset = 0;
    sm.index_count  = static_cast<GLuint>(indices.size());
    submeshes.push_back(sm);

    // Create & upload GL buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 unique_vertices.size() * sizeof(Vertex),
                 unique_vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(GLuint),
                 indices.data(),
                 GL_STATIC_DRAW);

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
}

Model::Model::Model(const ObjectLoader::OBJLoader& loader, const std::string& label)
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
    // upload matrices
    shader->set_mat4("uViewProj", view_projection);
    shader->set_mat4("uModel", world_transform);

    glBindVertexArray(vao);
    for (auto const& sm : submeshes) {
        shader->set_vec3("material.ambient", sm.mat.Ka);
        shader->set_vec3("material.diffuse", sm.mat.Kd);
        shader->set_vec3("material.specular", sm.mat.Ks);
        shader->set_vec3("material.emissive", sm.mat.Ke);
        shader->set_float("material.shininess", sm.mat.Ns);
        shader->set_float("material.opacity", sm.mat.d);
        shader->set_int("material.illumModel", sm.mat.illum);
        shader->set_float("material.ior", sm.mat.Ni);

        if(sm.mat.tex_Ka) {
            shader->set_texture("ambientMap", sm.mat.tex_Ka, GL_TEXTURE1);
            shader->set_bool   ("useAmbientMap", true);
        } else {
            shader->set_bool("useAmbientMap", false);
        }

        if(sm.mat.tex_Kd) {
            shader->set_texture("diffuseMap", sm.mat.tex_Kd, GL_TEXTURE0);
            shader->set_bool   ("useDiffuseMap", true);
        } else {
            shader->set_bool("useDiffuseMap", false);
        }

        if(sm.mat.tex_Ks) {
            shader->set_texture("specularMap", sm.mat.tex_Ks, GL_TEXTURE2);
            shader->set_bool   ("useSpecularMap", true);
        } else {
            shader->set_bool("useSpecularMap", false);
        }

        // normal map
        if(sm.mat.tex_Bump) {
            shader->set_texture("normalMap", sm.mat.tex_Bump, GL_TEXTURE3);
            shader->set_bool   ("useNormalMap", true);
        } else {
            shader->set_bool("useNormalMap", false);
        }

        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offsetPtr);
    }
    glBindVertexArray(0);
}

void Model::Model::draw_depth(Shader* shader) const {

    shader->set_mat4("uModel", world_transform);
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
    // 1) Initialize to extreme opposites
    glm::vec3 world_min(  FLT_MAX );
    glm::vec3 world_max( -FLT_MAX );

    // 2) Transform each unique-vertex into world space and accumulate
    for (auto const& v : unique_vertices) {
        glm::vec4 wc = world_transform * glm::vec4(v.position, 1.0f);
        glm::vec3 w  = glm::vec3(wc);
        world_min = glm::min(world_min, w);
        world_max = glm::max(world_max, w);
    }

    // 3) If truly planar (min == max in Y), pad by a tiny ε so your sphere
    //    test doesn’t see it as a zero-thickness plane.
    const float eps = 0.001f;
    if (glm::epsilonEqual(world_min.y, world_max.y, glm::epsilon<float>())) {
        world_min.y -= eps;
        world_max.y += eps;
    }

    // 4) Store
    aabbmin = world_min;
    aabbmax = world_max;
}

