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

void Model::Model::move_relative_to(const glm::vec3& direction) {
    
    glm::mat4 tf = local_transform;

    glm::vec3 forward = glm::normalize(glm::vec3(tf[2]));  // local Z
    glm::vec3 right   = glm::normalize(glm::vec3(tf[0]));  // local X
    glm::vec3 up      = glm::normalize(glm::vec3(tf[1]));  // local Y

    glm::vec3 move = direction.x * right + direction.y * up - direction.z * forward;

    tf = glm::translate(tf, move);
    this->set_local_transform(tf);

}


Model::Model::Model(const std::vector<glm::vec3>& positions,
                    const std::vector<glm::vec3>& normals,
                    const std::vector<glm::vec2>& texcoords,
                    const std::vector<GLuint>& indices,
                    const std::string& label,
                    const Material& mat)
    : local_transform(1.0f),
      world_transform(1.0f),
      localaabbmin(std::numeric_limits<float>::max()),
      localaabbmax(-std::numeric_limits<float>::lowest()),
      label(label)
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
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glGenBuffers(1, &ebo));

    GLCall(glBindVertexArray(vao));

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER,
                 unique_vertices.size() * sizeof(Vertex),
                 unique_vertices.data(),
                 GL_STATIC_DRAW));

    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(GLuint),
                 indices.data(),
                 GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, position)));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, texcoord)));
    GLCall(glEnableVertexAttribArray(2));
    GLCall(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, normal)));

    GLCall(glBindVertexArray(0));
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
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glGenBuffers(1, &ebo));

    GLCall(glBindVertexArray(vao));

    // VBO
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER,
                 unique_vertices.size() * sizeof(Vertex),
                 unique_vertices.data(),
                 GL_STATIC_DRAW));

    // EBO
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 all_indices.size() * sizeof(GLuint),
                 all_indices.data(),
                 GL_STATIC_DRAW));

    // attributes (pos, tex, norm) …
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, position)));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, texcoord)));
    GLCall(glEnableVertexAttribArray(2));
    GLCall(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, normal)));

    GLCall(glBindVertexArray(0));

    // compute local AABB
    for (auto const& v : unique_vertices) {
        localaabbmin = glm::min(localaabbmin, v.position);
        localaabbmax = glm::max(localaabbmax, v.position);
    }
}

Model::Model::~Model(){
    // Tear down GL objects in reverse order of creation:
    if (ebo) {
        GLCall(glDeleteBuffers(1, &ebo));
        ebo = 0;
    }
    if (vbo) {
        GLCall(glDeleteBuffers(1, &vbo));
        vbo = 0;
    }
    if (vao) {
        GLCall(glDeleteVertexArrays(1, &vao));
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

void Model::Model::draw_instanced(const glm::mat4& view, const glm::mat4& projection, Shader* shader) const{

    update_instance_data();

    shader->set_mat4("uView", view);
    shader->set_mat4("uProj", projection);
    shader->set_bool("uUseInstancing", true);

    GLCall(glBindVertexArray(vao));
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
        //if(sm.mat.tex_Bump) {
        //    shader->set_texture("normalMap", sm.mat.tex_Bump, GL_TEXTURE3);
        //    shader->set_bool   ("useNormalMap", true);
        //} else {
        //    shader->set_bool("useNormalMap", false);
        //}

        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElementsInstanced(
            GL_TRIANGLES,
            sm.index_count,
            GL_UNSIGNED_INT,
            offsetPtr,
            instance_transforms.size()
        ));
    }
    GLCall(glBindVertexArray(0));
}

void Model::Model::draw(const glm::mat4& view, const glm::mat4& projection, Shader* shader) const{
    // upload matrices
    shader->set_mat4("uView", view);
    shader->set_mat4("uProj", projection);
    shader->set_mat4("uModel", world_transform);
    shader->set_bool("uUseInstancing", false);

    GLCall(glBindVertexArray(vao));
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
        // if(sm.mat.tex_Bump) {
        //    shader->set_texture("normalMap", sm.mat.tex_Bump, GL_TEXTURE3);
        //    shader->set_bool   ("useNormalMap", true);
        // } else {
        //    shader->set_bool("useNormalMap", false);
        // }

        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offsetPtr));
    }
    GLCall(glBindVertexArray(0));
}

void Model::Model::draw_depth(Shader* shader) const {

    //GLCall(glEnable(GL_CULL_FACE));
    //GLCall(glCullFace(GL_FRONT));
    //GLCall(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));

    shader->set_mat4("uModel", world_transform);
    shader->set_bool("uUseInstancing", false);
    GLCall(glBindVertexArray(vao));
    for (auto const& sm : submeshes) {
        void* offset_ptr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElements(
            GL_TRIANGLES,
            sm.index_count,
            GL_UNSIGNED_INT,
            offset_ptr
        ));
    }
    //GLCall(glCullFace(GL_BACK));
    //GLCall(glColorMask(GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_TRUE));
    GLCall(glBindVertexArray(0));
}

void Model::Model::draw_depth_instanced(Shader* shader) const {
    update_instance_data();

    shader->set_bool("uUseInstancing", true);
    GLCall(glBindVertexArray(vao));


    for (auto const& sm : submeshes) {
        void* offset_ptr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElementsInstanced(
            GL_TRIANGLES,
            sm.index_count,
            GL_UNSIGNED_INT,
            offset_ptr,
            static_cast<GLsizei>(instance_transforms.size())
        ));
    }
    GLCall(glBindVertexArray(0));
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

static void compute_transformed_aabb(
    const glm::vec3& local_min,
    const glm::vec3& local_max,
    const glm::mat4& xf,
    glm::vec3& out_min,
    glm::vec3& out_max)
{
    // all 8 corners of the local box
    glm::vec3 corners[8] = {
        {local_min.x, local_min.y, local_min.z},
        {local_max.x, local_min.y, local_min.z},
        {local_min.x, local_max.y, local_min.z},
        {local_min.x, local_min.y, local_max.z},
        {local_max.x, local_max.y, local_min.z},
        {local_min.x, local_max.y, local_max.z},
        {local_max.x, local_min.y, local_max.z},
        {local_max.x, local_max.y, local_max.z},
    };

    out_min = glm::vec3( FLT_MAX);
    out_max = glm::vec3(-FLT_MAX);

    for (auto &c : corners) {
        glm::vec3 w = glm::vec3(xf * glm::vec4(c, 1.0f));
        out_min = glm::min(out_min, w);
        out_max = glm::max(out_max, w);
    }
}

void Model::Model::init_instancing(size_t max_instances) {
    // Generate the instance‐buffer
    GLCall(glGenBuffers(1, &instance_vbo));
    GLCall(glBindVertexArray(vao));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, instance_vbo));
    // allocate enough space for max_instances matrices
    GLCall(glBufferData(GL_ARRAY_BUFFER,
                 max_instances * sizeof(glm::mat4),
                 nullptr,
                 GL_DYNAMIC_DRAW));

    // Set up the four vec4 attributes (one per column of the mat4)
    constexpr GLuint loc = 3; // choose free attribute locations
    for (int i = 0; i < 4; ++i) {
        GLuint attrib = loc + i;
        GLCall(glEnableVertexAttribArray(attrib));
        GLCall(glVertexAttribPointer(attrib,
                              4,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(glm::mat4),
                              (void*)(sizeof(glm::vec4) * i)));
        // tell GL this is per-instance, not per-vertex:
        GLCall(glVertexAttribDivisor(attrib, 1));
    }
    is_instanced_ = true;
    GLCall(glBindVertexArray(0));
}

void Model::Model::update_instance_data() const{
    // Map & write only the portion we need (could also use glBufferSubData)
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, instance_vbo));
    void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr,
        instance_transforms.data(),
        instance_transforms.size() * sizeof(glm::mat4));
    GLCall(glUnmapBuffer(GL_ARRAY_BUFFER));
}

void Model::Model::add_instance_transform(const glm::mat4& xf) {
    instance_transforms.push_back(xf);

    glm::vec3 wmin, wmax;
    compute_transformed_aabb(localaabbmin, localaabbmax, xf, wmin, wmax);

    instance_aabb_min.push_back(wmin);
    instance_aabb_max.push_back(wmax);
}
