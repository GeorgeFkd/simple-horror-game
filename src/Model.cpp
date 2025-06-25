#include "Model.h"

static void print_vec3(glm::vec3 v) {
    std::cout << "(" << v.x << "," << v.y << "," << v.z << ")\n";
}

void Models::Model::debug_dump() const {
    std::cout << "Transforms for: " << label << "\n";
    if (is_instanced()) {
        for (size_t i = 0; i < instance_transforms.size(); i++) {
            std::cout << "===" << i << "====\n";
            std::cout << "Min: ";
            print_vec3(instance_aabb_min[i]);
            std::cout << "Max: ";
            print_vec3(instance_aabb_max[i]);
        }
    }
    // size_t total_indices   = 0;
    // size_t total_triangles = 0;
    // for (auto const& sm : submeshes) {
    //     total_indices   += sm.index_count;
    //     total_triangles += sm.index_count / 3;
    // }
    //
    // std::cout
    //   << "  >>> Model built: "
    //   << unique_vertices.size() << " unique vertices, "
    //   << total_triangles         << " triangles, "
    //   << total_indices           << " indices total\n"
    //   << "      Submeshes: "     << submeshes.size() << "\n"
    //   << "      AABB local min = ("
    //      << localaabbmin.x << ", "
    //      << localaabbmin.y << ", "
    //      << localaabbmin.z << ")\n"
    //   << "      AABB local max = ("
    //      << localaabbmax.x << ", "
    //      << localaabbmax.y << ", "
    //      << localaabbmax.z << ")" << std::endl;
}

void Models::Model::move_relative_to(const glm::vec3& direction) {

    glm::mat4 tf = local_transform;

    glm::vec3 forward = glm::normalize(glm::vec3(tf[2])); // local Z
    glm::vec3 right   = glm::normalize(glm::vec3(tf[0])); // local X
    glm::vec3 up      = glm::normalize(glm::vec3(tf[1])); // local Y

    glm::vec3 move = direction.x * right + direction.y * up - direction.z * forward;

    tf = glm::translate(tf, move);
    this->set_local_transform(tf);
}

Models::Model::Model(const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec2>& texcoords,
                     const std::vector<GLuint>& indices,
                     std::string label,
                     const Material& mat)
    : local_transform(1.0f)
    , world_transform(1.0f)
    , localaabbmin(std::numeric_limits<float>::max())
    , localaabbmax(std::numeric_limits<float>::lowest())
    , label(std::move(label))
{

    unique_vertices.reserve(positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        Vertex vert;
        vert.position = positions[i];
        vert.normal   = (i < normals.size())
                        ? normals[i]
                        : glm::vec3(0.0f, 1.0f, 0.0f);
        vert.texcoord = (i < texcoords.size())
                        ? texcoords[i]
                        : glm::vec2(0.0f);

        vert.tangent  = glm::vec4(0.0f);
        unique_vertices.push_back(vert);

        localaabbmin = glm::min(localaabbmin, vert.position);
        localaabbmax = glm::max(localaabbmax, vert.position);
    }

    SubMesh sm;
    sm.mat          = mat;
    sm.index_offset = 0;
    sm.index_count  = static_cast<GLuint>(indices.size());
    submeshes.push_back(sm);

    std::vector<glm::vec3> tan1(unique_vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(unique_vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        GLuint i0 = indices[i + 0];
        GLuint i1 = indices[i + 1];
        GLuint i2 = indices[i + 2];

        const auto& v0 = unique_vertices[i0];
        const auto& v1 = unique_vertices[i1];
        const auto& v2 = unique_vertices[i2];

        auto [T, B] = calculate_tangent_bitangent(v0, v1, v2);

        tan1[i0] += T;  tan1[i1] += T;  tan1[i2] += T;
        tan2[i0] += B;  tan2[i1] += B;  tan2[i2] += B;
    }

    for (size_t i = 0; i < unique_vertices.size(); ++i) {
        orthogonalize_and_normalize_tb(unique_vertices[i], tan1, tan2, i);
    }

    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glGenBuffers(1,         &vbo));
    GLCall(glGenBuffers(1,         &ebo));

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
    GLCall(glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, position)));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, texcoord)));

    GLCall(glEnableVertexAttribArray(2));
    GLCall(glVertexAttribPointer(
        2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, normal)));

    GLCall(glEnableVertexAttribArray(3));
    GLCall(glVertexAttribPointer(
        3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, tangent)));

    GLCall(glBindVertexArray(0));
}

Models::Model::Model(const std::string& objFile, const std::string& label)
    : local_transform(1.0f), world_transform(1.0f), localaabbmin(std::numeric_limits<float>::max()),
      localaabbmax(-std::numeric_limits<float>::max()), label(label) {
    ObjectLoader::OBJLoader loader;
    loader.read_from_file(objFile);

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
    all_indices.reserve(std::accumulate(buckets.begin(), buckets.end(), 0u,
                                        [](auto sum, auto& p) { return sum + p.second.size(); }));

    for (auto& [material_id, indexes] : buckets) {
        SubMesh sm;
        if (material_id >= 0) {
            sm.mat = loader.m_materials[material_id]; // assumes same Material layout
        } else {
            sm.mat = Material{};
        }
        sm.index_offset = (GLuint)all_indices.size();
        sm.index_count  = (GLuint)indexes.size();

        all_indices.insert(all_indices.end(), indexes.begin(), indexes.end());
        submeshes.push_back(sm);
    }

    // storage for accumulating each shared vertex's contributions
    std::vector<glm::vec3> tan1(unique_vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(unique_vertices.size(), glm::vec3(0.0f));
    
    for (size_t i = 0; i + 2 < all_indices.size(); i += 3) {
        GLuint i0 = all_indices[i+0];
        GLuint i1 = all_indices[i+1];
        GLuint i2 = all_indices[i+2];
    
        auto& v0 = unique_vertices[i0];
        auto& v1 = unique_vertices[i1];
        auto& v2 = unique_vertices[i2];
    
        auto [T, B] = calculate_tangent_bitangent(v0, v1, v2);
    
        tan1[i0] += T;  tan1[i1] += T;  tan1[i2] += T;
        tan2[i0] += B;  tan2[i1] += B;  tan2[i2] += B;
    }

    for(int i = 0; i < unique_vertices.size(); i++){
        orthogonalize_and_normalize_tb(unique_vertices[i], tan1, tan2, i);
    }

    // create & upload VAO/VBO/EBO
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glGenBuffers(1, &ebo));

    GLCall(glBindVertexArray(vao));

    // VBO
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, unique_vertices.size() * sizeof(Vertex),
                        unique_vertices.data(), GL_STATIC_DRAW));

    // EBO
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, all_indices.size() * sizeof(GLuint),
                        all_indices.data(), GL_STATIC_DRAW));

    // attributes (pos, tex, norm) …
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                 (void*)offsetof(Vertex, position)));
    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                 (void*)offsetof(Vertex, texcoord)));
    GLCall(glEnableVertexAttribArray(2));
    GLCall(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                 (void*)offsetof(Vertex, normal)));
    GLCall(glEnableVertexAttribArray(3));
    GLCall(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, tangent)));

    GLCall(glBindVertexArray(0));

    // compute local AABB
    for (auto const& v : unique_vertices) {
        localaabbmin = glm::min(localaabbmin, v.position);
        localaabbmax = glm::max(localaabbmax, v.position);
    }
}

Models::Model::~Model() {
    // Tear down instancing first (reverse of creation)
    if (instance_vbo) {
        GLCall(glDeleteBuffers(1, &instance_vbo));
        instance_vbo = 0;
    }
    is_instanced_ = false;

    // Clear all CPU‐side instance arrays
    instance_suffixes.clear();
    instance_transforms.clear();
    instance_aabb_min.clear();
    instance_aabb_max.clear();
    instance_modifications.clear();

    // Then tear down your regular VAO/VBO/EBO in reverse creation order
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

void Models::Model::orthogonalize_and_normalize_tb(
    Models::Vertex& vertex,
    const std::vector<glm::vec3>& accumulated_tangent, 
    const std::vector<glm::vec3>& accumulated_bitangent,
    const size_t index
) {
    const glm::vec3& normal    = vertex.normal;
    const glm::vec3& tangent   = accumulated_tangent[index];
    const glm::vec3& bitangent = accumulated_bitangent[index];

    //
    // Gram–Schmidt orthogonalize the tangent against the normal
    glm::vec3 orth_tangent = glm::normalize(
        tangent - normal * glm::dot(normal, tangent)
    );

    // Compute handedness (±1) so we can reconstruct the bi‐tangent in‐shader if desired
    float handedness = (glm::dot(glm::cross(normal, orth_tangent), bitangent) < 0.0f)
                       ? -1.0f
                       : 1.0f;

    // Store the results back into the vertex
    vertex.tangent   = glm::vec4(orth_tangent, handedness);
}

std::pair<glm::vec3, glm::vec3> Models::Model::calculate_tangent_bitangent(
    Models::Vertex v0, 
    Models::Vertex v1, 
    Models::Vertex v2
){

    glm::vec3 edge1 = v1.position - v0.position;
    glm::vec3 edge2 = v2.position - v0.position;
    glm::vec2 uv0   = v0.texcoord;
    glm::vec2 uv1   = v1.texcoord;
    glm::vec2 uv2   = v2.texcoord;

    glm::vec2 delta_uv1 = uv1 - uv0;
    glm::vec2 delta_uv2 = uv2 - uv0;
    // Compute the inverse of the determinant of the UV matrix (Δ)
    // This is equivalent to: Δ = 1 / (s1 * t2 - s2 * t1)
    float r = 1.0f / (delta_uv1.x * delta_uv2.y  - delta_uv2.x * delta_uv1.y);

    // Compute the tangent direction vector (T)
    // This solves: T = (t2 * Q1 - t1 * Q2) / Δ
    glm::vec3 tangent = {0.0f, 0.0f, 0.0f};
    // Compute the bitangent direction vector (B)
    // This solves: B = (-s2 * Q1 + s1 * Q2) / Δ
    glm::vec3 bitangent = {0.0f, 0.0f, 0.0f};

    tangent.x = r * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
    tangent.y = r * (delta_uv2.y * edge1.y- delta_uv1.y * edge2.y);
    tangent.z = r * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);
    bitangent.x = r * (-delta_uv2.x * edge1.x + delta_uv1.x * edge2.x);
    bitangent.y = r * (-delta_uv2.x * edge1.y + delta_uv1.x * edge2.y);
    bitangent.z = r * (-delta_uv2.x * edge1.z + delta_uv1.x * edge2.z);

    return {tangent, bitangent};
}

void Models::Model::add_child(Model* child) {
    children.push_back(child);
}

void Models::Model::set_local_transform(const glm::mat4& local_transform) {
    this->local_transform = local_transform;
}

void Models::Model::update_world_transform(const glm::mat4& parent_transform) {
    world_transform = parent_transform * local_transform;

    compute_aabb();
    for (Model* child : children) {
        child->update_world_transform(world_transform);
    }
}
//I could remove this from the public API
//and have it be an impl detail, as both draws are called with the same params
void Models::Model::draw_instanced(const glm::mat4& view, const glm::mat4& projection,
                                   std::shared_ptr<Shader> shader){

    //CARE WITH THIS IS MIGHT CAUSE A BUG
    //if(instance_data_dirty) update_instance_data();
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
        shader->set_bool("material.useBumpMap", sm.mat.use_bump_map);

        if (sm.mat.tex_Ka) {
            shader->set_texture("ambientMap", sm.mat.tex_Ka, GL_TEXTURE1);
            shader->set_bool("useAmbientMap", true);
        } else {
            shader->set_bool("useAmbientMap", false);
        }

        if (sm.mat.tex_Kd) {
            shader->set_texture("diffuseMap", sm.mat.tex_Kd, GL_TEXTURE2);
            shader->set_bool("useDiffuseMap", true);
        } else {
            shader->set_bool("useDiffuseMap", false);
        }

        if (sm.mat.tex_Ks) {
            shader->set_texture("specularMap", sm.mat.tex_Ks, GL_TEXTURE3);
            shader->set_bool("useSpecularMap", true);
        } else {
            shader->set_bool("useSpecularMap", false);
        }

        if(sm.mat.tex_Bump) {
           shader->set_texture("bumpMap", sm.mat.tex_Bump, GL_TEXTURE4);
           shader->set_float("bumpScale", 4.0f);
        } 

        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElementsInstanced(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offsetPtr,
                                       instance_transforms.size()));
    }
    GLCall(glBindVertexArray(0));
}

void Models::Model::draw(const glm::mat4& view, const glm::mat4& projection,
                         std::shared_ptr<Shader> shader){
    if(is_instanced_) {
        draw_instanced(view,projection,shader);
        return;
    }

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
        shader->set_bool("material.useBumpMap", sm.mat.use_bump_map);

        if (sm.mat.tex_Ka) {
            shader->set_texture("ambientMap", sm.mat.tex_Ka, GL_TEXTURE1);
            shader->set_bool("useAmbientMap", true);
        } else {
            shader->set_bool("useAmbientMap", false);
        }

        if (sm.mat.tex_Kd) {
            shader->set_texture("diffuseMap", sm.mat.tex_Kd, GL_TEXTURE2);
            shader->set_bool("useDiffuseMap", true);
        } else {
            shader->set_bool("useDiffuseMap", false);
        }

        if (sm.mat.tex_Ks) {
            shader->set_texture("specularMap", sm.mat.tex_Ks, GL_TEXTURE3);
            shader->set_bool("useSpecularMap", true);
        } else {
            shader->set_bool("useSpecularMap", false);
        }

        if(sm.mat.tex_Bump) {
           shader->set_texture("bumpMap", sm.mat.tex_Bump, GL_TEXTURE4);
           shader->set_float("bumpScale", 4.0f);
        } 

        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offsetPtr));
    }
    GLCall(glBindVertexArray(0));
}

void Models::Model::draw_depth(std::shared_ptr<Shader> shader){

    // GLCall(glEnable(GL_CULL_FACE));
    // GLCall(glCullFace(GL_FRONT));
    // GLCall(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));

    shader->set_mat4("uModel", world_transform);
    shader->set_bool("uUseInstancing", false);
    GLCall(glBindVertexArray(vao));
    for (auto const& sm : submeshes) {
        void* offset_ptr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offset_ptr));
    }
    // GLCall(glCullFace(GL_BACK));
    // GLCall(glColorMask(GL_TRUE,  GL_TRUE,  GL_TRUE,  GL_TRUE));
    GLCall(glBindVertexArray(0));
}

void Models::Model::draw_depth_instanced(std::shared_ptr<Shader> shader){
    //if(instance_data_dirty) update_instance_data();
    update_instance_data();
    shader->set_bool("uUseInstancing", true);
    shader->set_mat4("uModel", world_transform);

    GLCall(glBindVertexArray(vao));

    for (auto const& sm : submeshes) {
        void* offset_ptr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElementsInstanced(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offset_ptr,
                                       static_cast<GLsizei>(instance_transforms.size())));
    }
    GLCall(glBindVertexArray(0));
}

void Models::Model::compute_aabb() {
    // 1) Initialize to extreme opposites
    glm::vec3 world_min(FLT_MAX);
    glm::vec3 world_max(-FLT_MAX);

    // 2) Transform each unique-vertex into world space and accumulate
    for (auto const& v : unique_vertices) {
        glm::vec4 wc = world_transform * glm::vec4(v.position, 1.0f);
        glm::vec3 w  = glm::vec3(wc);
        world_min    = glm::min(world_min, w);
        world_max    = glm::max(world_max, w);
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

void Models::Model::compute_transformed_aabb(
                                     const glm::mat4& xf, glm::vec3& out_min, glm::vec3& out_max) {
    // all 8 corners of the local box
    glm::vec3 corners[8] = {
            {localaabbmin.x, localaabbmin.y, localaabbmin.z},
            {localaabbmin.x, localaabbmin.y, localaabbmax.z},
            {localaabbmin.x, localaabbmax.y, localaabbmin.z},
            {localaabbmin.x, localaabbmax.y, localaabbmax.z},

            {localaabbmax.x, localaabbmin.y, localaabbmin.z},
            {localaabbmax.x, localaabbmin.y, localaabbmax.z},
            {localaabbmax.x, localaabbmax.y, localaabbmin.z},
            {localaabbmax.x, localaabbmax.y, localaabbmax.z},
    };

    out_min = glm::vec3(FLT_MAX);
    out_max = glm::vec3(-FLT_MAX);

    for (auto& c : corners) {
        glm::vec3 w = glm::vec3(xf * glm::vec4(c, 1.0f));
        out_min = glm::min(out_min, w);
        out_max = glm::max(out_max, w);
    }

    const float eps = 0.001f;
    if (glm::epsilonEqual(out_min.y, out_max.y, glm::epsilon<float>())) {
        out_min.y -= eps;
        out_max.y += eps;
    }
}

void Models::Model::init_instancing(size_t max_instances) {
    // Generate the instance‐buffer
    GLCall(glGenBuffers(1, &instance_vbo));
    GLCall(glBindVertexArray(vao));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, instance_vbo));
    // allocate enough space for max_instances matrices
    GLCall(
        glBufferData(GL_ARRAY_BUFFER, max_instances * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW));

    // Set up the four vec4 attributes (one per column of the mat4)
    constexpr GLuint loc = 4; // choose free attribute locations
    for (int i = 0; i < 4; ++i) {
        GLuint attrib = loc + i;
        GLCall(glEnableVertexAttribArray(attrib));
        GLCall(glVertexAttribPointer(attrib, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                                     (void*)(sizeof(glm::vec4) * i)));
        // tell GL this is per-instance, not per-vertex:
        GLCall(glVertexAttribDivisor(attrib, 1));
    }
    is_instanced_ = true;
    instance_suffixes.reserve(max_instances);
    instance_transforms.reserve(max_instances);
    instance_aabb_min.reserve(max_instances);
    instance_aabb_max.reserve(max_instances);
    instance_modifications.reserve(max_instances);
    instance_in_frustum.reserve(max_instances);
    GLCall(glBindVertexArray(0));
}

//void Models::Model::update_instance_data() const {
//    // Map & write only the portion we need (could also use glBufferSubData)
//    GLCall(glBindBuffer(GL_ARRAY_BUFFER, instance_vbo));
//    void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
//    memcpy(ptr, instance_transforms.data(), instance_transforms.size() * sizeof(glm::mat4));
//    GLCall(glUnmapBuffer(GL_ARRAY_BUFFER));
//}

void Models::Model::update_instance_data(){
    // build a temporary list of only the active transforms
    std::vector<glm::mat4> active;
    active.reserve(instance_transforms.size());
    for (size_t i = 0; i < instance_transforms.size(); ++i) {

        if(instance_modifications[i] == InstanceModifiedTypes::REMOVED){
            continue;
        }

        if(!instance_in_frustum[i]){
            continue;
        }

        active.push_back(instance_transforms[i]);
    }

    // upload only the active list
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, instance_vbo));
    GLCall(glBufferData(
      GL_ARRAY_BUFFER,
      active.size() * sizeof(glm::mat4),
      active.data(),
      GL_STREAM_DRAW));
    // GLCall(glUnmapBuffer(GL_ARRAY_BUFFER));
    // remember how many instances we’ll actually draw
    gl_instance_count = (GLuint)active.size();
    instance_data_dirty = false;
}

void Models::Model::add_instance_transform(const glm::mat4& xf, const std::string& suffix){
    instance_transforms.push_back(xf);

    glm::vec3 wmin, wmax;
    compute_transformed_aabb(xf, wmin, wmax);

    instance_aabb_min.push_back(wmin);
    instance_aabb_max.push_back(wmax);
    instance_suffixes.push_back(suffix);
    instance_modifications.push_back(InstanceModifiedTypes::NOT_MODIFIED);
    instance_in_frustum.push_back(true);
    instance_data_dirty = true;
}

std::pair<float,int> Models::Model::distance_from_point_using_AABB(const glm::vec3& point)
{
    static const glm::vec3 convenience_offset{0.0f, -0.6f, 0.0f};
    glm::vec3 offset_cen = point + convenience_offset;

    // non-instanced: just one AABB, instance = -1
    if (!is_instanced()) {
        glm::vec3 closest = glm::clamp(offset_cen, aabbmin, aabbmax);
        float d2 = glm::length2(closest - offset_cen);
        return { d2, -1 };
    }

    // instanced: find the instance with the smallest distance
    float best_d2 = std::numeric_limits<float>::max();
    int best_idx = -1;

    for (int i = 0; i < get_instance_count(); ++i) {
        if (instance_modifications[i] == InstanceModifiedTypes::REMOVED)
            continue;
        const auto& min_i = get_instance_aabb_min(i);
        const auto& max_i = get_instance_aabb_max(i);
        glm::vec3 closest = glm::clamp(offset_cen, min_i, max_i);
        float d2 = glm::length2(closest - offset_cen);

        if (d2 < best_d2) {
            best_d2 = d2;
            best_idx = i;
        }
    }

    return { best_d2, best_idx };
}

std::pair<bool, int> Models::Model::intersect_sphere_aabb(const glm::vec3& point, float radius){
    auto[squared_distance, instance_index] = distance_from_point_using_AABB(point);
    return {squared_distance <= radius * radius, instance_index};
}

std::tuple<std::string, bool, float> Models::Model::is_closer_than_current_model(const glm::vec3& point_to_check, float current_distance_to_closest_model){

    auto [squared_distance, instance_index] = distance_from_point_using_AABB(point_to_check);

    if(instance_index == - 1){
        return  {name(), squared_distance < current_distance_to_closest_model, squared_distance};
    }

    return {name(instance_index), squared_distance < current_distance_to_closest_model, squared_distance};
}

bool Models::Model::aabb_in_frustum(const std::array<glm::vec4,6>& P, const glm::vec3& minB, const glm::vec3& maxB) const{
    for (auto& plane : P) {
        // pick the “positive‐vertex” for this plane normal
        glm::vec3 n(plane);
        glm::vec3 positive = {
            n.x > 0.0f ? maxB.x : minB.x,
            n.y > 0.0f ? maxB.y : minB.y,
            n.z > 0.0f ? maxB.z : minB.z
        };
        // if that vertex is outside, the whole box is outside
        if (glm::dot(n, positive) + plane.w < 0.0f)
            return false;
    }
    return true;
}

void Models::Model::in_frustum(const std::array<glm::vec4,6>& P){
    // clear previous state
    inside_frustum_ = false;
    if (is_instanced())
        instance_in_frustum.assign(get_instance_count(), false);

    // non-instanced: single AABB
    if (!is_instanced()) {
        inside_frustum_ = aabb_in_frustum(P, aabbmin, aabbmax);
        return;
    }

    // instanced: test each live instance
    int N = get_instance_count();
    for (int i = 0; i < N; ++i) {
        if (instance_modifications[i] == InstanceModifiedTypes::REMOVED)
            continue;

        auto minB = get_instance_aabb_min(i);
        auto maxB = get_instance_aabb_max(i);
        bool inside = aabb_in_frustum(P, minB, maxB);
        instance_in_frustum[i] = inside;
        if (inside)
            inside_frustum_ = true;  // model is “in” if any instance is
    }
}



void Models::Model::remove_instance_transform(const std::string& suffix){
    auto it = std::find(instance_suffixes.begin(), instance_suffixes.end(), suffix);
    if (it == instance_suffixes.end()) {
        std::cerr << "Instance suffix not found: " << suffix << "\n";
        return;
    }

    size_t i = std::distance(instance_suffixes.begin(), it);
    instance_modifications[i] = InstanceModifiedTypes::REMOVED;
    instance_in_frustum[i] = false;
    instance_data_dirty = true;
}

Models::Model Models::createFloor(float roomSize) {

    float                  y           = 0.0f;
    std::vector<glm::vec3> floor_verts = {{-roomSize, y, -roomSize},
                                          {-roomSize, y, roomSize},
                                          {roomSize, y, roomSize},
                                          {roomSize, y, -roomSize}};
    std::vector<glm::vec3> floor_normals(4, glm::vec3(0, 1, 0));
    std::vector<glm::vec2> floor_uvs     = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    std::vector<GLuint>    floor_indices = {0, 1, 2, 0, 2, 3};

    Material floor_material;
    floor_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f); // dark ambient
    floor_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f); // brown diffuse
    floor_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f); // small specular
    floor_material.Ns    = 16.0f;                          // shininess
    floor_material.d     = 1.0f;                           // opacity
    floor_material.illum = 2;                              // standard Phong
    floor_material.use_bump_map = false;

    return Models::Model(floor_verts, floor_normals, floor_uvs, floor_indices, std::move("Floor"),
                         floor_material);
}

Models::Model Models::createCeiling(float roomSize, float height) {
    std::vector<glm::vec3> floor_verts = {{-roomSize, height, -roomSize},
                                          {-roomSize, height, roomSize},
                                          {roomSize, height, roomSize},
                                          {roomSize, height, -roomSize}};
    std::vector<glm::vec3> floor_normals(4, glm::vec3(0, -1, 0));
    std::vector<glm::vec2> floor_uvs = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    // the indices are changed to agree with the normals 0,-1,0 as otherwise it is discarded
    std::vector<GLuint> floor_indices = {0, 2, 1, 0, 3, 2};

    Material floor_material;
    floor_material.Ka           = glm::vec3(0.15f, 0.07f, 0.02f); // dark ambient
    floor_material.Kd           = glm::vec3(0.59f, 0.29f, 0.00f); // brown diffuse
    floor_material.Ks           = glm::vec3(0.05f, 0.04f, 0.03f); // small specular
    floor_material.Ns           = 16.0f;                          // shininess
    floor_material.d            = 1.0f;                           // opacity
    floor_material.illum        = 2;                              // standard Phong
    floor_material.use_bump_map = false;

    return Models::Model(floor_verts, floor_normals, floor_uvs, floor_indices, std::move("Ceiling"),
                         floor_material);
}


Models::Model Models::createWallFront(float roomSize, float roomHeight) {
    // roomSize == half‐width & half‐depth of your room; roomHeight is the height of the wall.
    float z0 = roomSize;
    float y0 = 0.0f;
    float y1 = roomHeight;

    // bottom‐left, bottom‐right, top‐right, top‐left (CCW when viewed from -Z side)
    std::vector<glm::vec3> wall_verts = {
        { -roomSize, y0, z0 },  // BL
        { +roomSize, y0, z0 },  // BR
        { +roomSize, y1, z0 },  // TR
        { -roomSize, y1, z0 }   // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, -1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {
        0, 2, 1,
        0, 3, 2
    };

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns    = 16.0f;
    wall_material.d     = 1.0f;
    wall_material.illum = 2;

    wall_material.use_bump_map = false;
    return Models::Model(
        wall_verts,
        wall_normals,
        wall_uvs,
        wall_indices,
        "WallFront",
        wall_material
    );
}
Models::Model Models::createWallRight(float roomSize, float roomHeight) {
    // roomSize == half‐width & half‐depth of your room; roomHeight is the height of the wall.
    float z0 = roomSize;
    float y0 = 0.0f;
    float y1 = roomHeight;

    // bottom‐left, bottom‐right, top‐right, top‐left (CCW when viewed from -Z side)
    std::vector<glm::vec3> wall_verts = {
        { roomSize, y0, -z0 },  // BL
        { roomSize, y0, z0 },  // BR
        { roomSize, y1, z0 },  // TR
        { roomSize, y1, -z0 }   // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, -1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {
        0, 1, 2,
        0, 2, 3
    };

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns    = 16.0f;
    wall_material.d     = 1.0f;
    wall_material.illum = 2;

    wall_material.use_bump_map = false;
    return Models::Model(
        wall_verts,
        wall_normals,
        wall_uvs,
        wall_indices,
        "WallRight",
        wall_material
    );
}
Models::Model Models::createWallLeft(float roomSize, float roomHeight) {
    // roomSize == half‐width & half‐depth of your room; roomHeight is the height of the wall.
    float z0 = roomSize;
    float y0 = 0.0f;
    float y1 = roomHeight;

    // bottom‐left, bottom‐right, top‐right, top‐left (CCW when viewed from -Z side)
    std::vector<glm::vec3> wall_verts = {
        { -roomSize, y0, -z0 },  // BL
        { -roomSize, y0, z0 },  // BR
        { -roomSize, y1, z0 },  // TR
        { -roomSize, y1, -z0 }   // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, 1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {
        0, 2, 1,
        0, 3, 2
    };

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns    = 16.0f;
    wall_material.d     = 1.0f;
    wall_material.illum = 2;

    wall_material.use_bump_map = false;
    return Models::Model(
        wall_verts,
        wall_normals,
        wall_uvs,
        wall_indices,
        "WallLeft",
        wall_material
    );
}


Models::Model Models::createWallBack(float roomSize, float roomHeight) {
    // roomSize == half‐width & half‐depth of your room; roomHeight is the height of the wall.
    float z0 = -roomSize;
    float y0 = 0.0f;
    float y1 = roomHeight;

    // bottom‐left, bottom‐right, top‐right, top‐left (CCW when viewed from -Z side)
    std::vector<glm::vec3> wall_verts = {
        { -roomSize, y0, z0 },  // BL
        { +roomSize, y0, z0 },  // BR
        { +roomSize, y1, z0 },  // TR
        { -roomSize, y1, z0 }   // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, 1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {
        0, 1, 2,
        0, 2, 3
    };

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns    = 16.0f;
    wall_material.d     = 1.0f;
    wall_material.illum = 2;
    wall_material.use_bump_map = false;

    return Models::Model(
        wall_verts,
        wall_normals,
        wall_uvs,
        wall_indices,
        "WallBack",
        wall_material
    );
}
