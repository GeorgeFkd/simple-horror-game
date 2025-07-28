#include "Model.h"
#include <limits>
#include <memory>

Models::MData::~MData() {
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

static void print_vec3(glm::vec3 v) {
    std::cout << "(" << v.x << "," << v.y << "," << v.z << ")\n";
}

void Models::Model::debug_dump() const {
    std::cout << "Transforms for: " << model_instance.label << "\n";
    size_t total_indices   = 0;
    size_t total_triangles = 0;
    for (auto const& sm : model_data->submeshes) {
        total_indices += sm.index_count;
        total_triangles += sm.index_count / 3;
    }

    std::cout << "  >>> Model built: " << model_data->unique_vertices.size() << " unique vertices, "
              << total_triangles << " triangles, " << total_indices << " indices total\n"
              << "      Submeshes: " << model_data->submeshes.size() << "\n"
              << "      AABB local min = (" << model_data->localaabbmin.x << ", "
              << model_data->localaabbmin.y << ", " << model_data->localaabbmin.z << ")\n"
              << "      AABB local max = (" << model_data->localaabbmax.x << ", "
              << model_data->localaabbmax.y << ", " << model_data->localaabbmax.z << ")"
              << std::endl;
}

void Models::Model::move_relative_to(const glm::vec3& direction) {

    glm::mat4 tf = model_instance.local_transform;

    glm::vec3 forward = glm::normalize(glm::vec3(tf[2])); // local Z
    glm::vec3 right   = glm::normalize(glm::vec3(tf[0])); // local X
    glm::vec3 up      = glm::normalize(glm::vec3(tf[1])); // local Y

    glm::vec3 move = direction.x * right + direction.y * up - direction.z * forward;

    tf = glm::translate(tf, move);
    this->set_local_transform(tf);
}

std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> Models::Model::prepare_bitangents() {
    std::vector<glm::vec3> tan1(model_data->unique_vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(model_data->unique_vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i + 2 < model_data->indices.size(); i += 3) {
        GLuint i0 = model_data->indices[i + 0];
        GLuint i1 = model_data->indices[i + 1];
        GLuint i2 = model_data->indices[i + 2];

        const auto& v0 = model_data->unique_vertices[i0];
        const auto& v1 = model_data->unique_vertices[i1];
        const auto& v2 = model_data->unique_vertices[i2];

        auto [T, B] = calculate_tangent_bitangent(v0, v1, v2);

        tan1[i0] += T;
        tan1[i1] += T;
        tan1[i2] += T;
        tan2[i0] += B;
        tan2[i1] += B;
        tan2[i2] += B;
    }
    return std::make_pair(tan1, tan2);
}

Models::Model::Model(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec2>& texcoords, const std::vector<GLuint>& indices,
                     std::string label, const Material& mat)

{
    model_instance.local_transform = 1.0f;
    model_instance.world_transform = 1.0f;
    model_instance.label           = std::move(label);
    model_data                     = std::make_shared<MData>();

    model_data->indices = indices;
    model_data->unique_vertices.reserve(positions.size());

    auto default_normal   = glm::vec3(0.0f, 1.0f, 0.0f);
    auto default_texcoord = glm::vec2(0.0f);
    for (size_t i = 0; i < positions.size(); ++i) {
        Vertex vert;
        vert.position = positions[i];
        vert.normal   = (i < normals.size()) ? normals[i] : default_normal;
        vert.texcoord = (i < texcoords.size()) ? texcoords[i] : default_texcoord;

        vert.tangent = glm::vec4(0.0f);
        model_data->unique_vertices.push_back(vert);
    }

    SubMesh sm;
    sm.mat          = mat;
    sm.index_offset = 0;
    sm.index_count  = static_cast<GLuint>(model_data->indices.size());
    model_data->submeshes.push_back(sm);

    auto [tan1, tan2] = prepare_bitangents();
    for (size_t i = 0; i < model_data->unique_vertices.size(); ++i) {
        orthogonalize_and_normalize_tb(model_data->unique_vertices[i], tan1, tan2, i);
    }
    // when the caching is complete those 2 will also be cached
    reserve_open_gl_memory();
    initialize_local_aabb();
}

void Models::Model::initialize_local_aabb() {
    model_data->localaabbmin = glm::vec3(std::numeric_limits<float>::max());
    model_data->localaabbmax = glm::vec3(-std::numeric_limits<float>::max());
    for (auto const& v : model_data->unique_vertices) {
        model_data->localaabbmin = glm::min(model_data->localaabbmin, v.position);
        model_data->localaabbmax = glm::max(model_data->localaabbmax, v.position);
    }
}
void Models::Model::reserve_open_gl_memory() {
    GLCall(glGenVertexArrays(1, &model_data->vao));
    GLCall(glGenBuffers(1, &model_data->vbo));
    GLCall(glGenBuffers(1, &model_data->ebo));

    GLCall(glBindVertexArray(model_data->vao));

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, model_data->vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, model_data->unique_vertices.size() * sizeof(Vertex),
                        model_data->unique_vertices.data(), GL_STATIC_DRAW));

    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model_data->ebo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, model_data->indices.size() * sizeof(GLuint),
                        model_data->indices.data(), GL_STATIC_DRAW));

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
}

Models::Model::Model(const Model& model_to_replicate, std::string model_name, glm::mat4 transform) {
    model_data                     = model_to_replicate.model_data;
    model_instance.label           = std::move(model_name);
    model_instance.local_transform = transform;
    interactable                   = model_to_replicate.interactable;
    // opengl memory is already initialised and local aabb boundaries are already initialised in
    // model data
}

Models::Model::Model(const std::string& objFile, std::string label) {
    model_instance.local_transform = 1.0f;
    model_instance.world_transform = 1.0f;
    model_instance.label           = std::move(label);

    auto it = model_registry.find(objFile);
    if (it != model_registry.end()) {
        std::cout << "Model: " << objFile << " already exists in model registry \n";
        model_data = it->second;
        return;
    } else {
        std::cout << "Model: " << objFile << " not in registry\n";
        std::cout << "Registry size: " << model_registry.size() << "\n";
    }

    ObjectLoader::OBJLoader loader;
    auto                    obj_model_data = loader.read_from_file(objFile);

    model_data = std::make_shared<MData>();
    // build unique_vertices & a cache
    std::unordered_map<Vertex, GLuint, VertexHasher> cache;
    cache.reserve(obj_model_data->m_faces.size() * 4);

    // bucket indices by material_id
    std::unordered_map<int, std::vector<GLuint>> buckets;

    auto default_texcoord = glm::vec2(0.0f, 0.0f);
    auto default_normal   = glm::vec3(0.0f, 0.0f, 1.0f);
    auto add_vertex       = [&](int vi, int ti, int ni) {
        Vertex vert;
        vert.position = glm::vec3(obj_model_data->m_vertices[vi]);
        if (ti >= 0 && ti < (int)obj_model_data->m_texture_coords.size()) {
            vert.texcoord = obj_model_data->m_texture_coords[ti];
        } else {
            vert.texcoord = default_texcoord;
        }

        if (ni >= 0 && ni < (int)obj_model_data->m_vertex_normals.size()) {
            vert.normal = obj_model_data->m_vertex_normals[ni];
        } else {
            vert.normal = default_normal;
        }

        auto [it, inserted] = cache.emplace(vert, (GLuint)this->model_data->unique_vertices.size());
        if (inserted) {
            this->model_data->unique_vertices.push_back(vert);
        }
        return it->second;
    };

    for (auto const& face : obj_model_data->m_faces) {
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
    model_data->indices.reserve(
        std::accumulate(buckets.begin(), buckets.end(), 0u,
                        [](auto sum, auto& p) { return sum + p.second.size(); }));

    for (auto& [material_id, indexes] : buckets) {
        SubMesh sm;
        if (material_id >= 0) {
            sm.mat = obj_model_data->m_materials[material_id];
        } else {
            sm.mat = Material{};
        }
        sm.index_offset = (GLuint)model_data->indices.size();
        sm.index_count  = (GLuint)indexes.size();

        model_data->indices.insert(model_data->indices.end(), indexes.begin(), indexes.end());
        model_data->submeshes.push_back(sm);
    }

    // storage for accumulating each shared vertex's contributions
    auto [tan1, tan2] = prepare_bitangents();
    for (int i = 0; i < model_data->unique_vertices.size(); i++) {
        orthogonalize_and_normalize_tb(model_data->unique_vertices[i], tan1, tan2, i);
    }

    reserve_open_gl_memory();
    initialize_local_aabb();

    assert(model_data->vao != 0 && model_data->ebo != 0 && model_data->vbo != 0);
    model_registry[objFile] = model_data;
}

void Models::Model::orthogonalize_and_normalize_tb(
    Models::Vertex& vertex, const std::vector<glm::vec3>& accumulated_tangent,
    const std::vector<glm::vec3>& accumulated_bitangent, const size_t index) {
    const glm::vec3& normal    = vertex.normal;
    const glm::vec3& tangent   = accumulated_tangent[index];
    const glm::vec3& bitangent = accumulated_bitangent[index];

    //
    // Gram–Schmidt orthogonalize the tangent against the normal
    glm::vec3 orth_tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));

    // Compute handedness (±1) so we can reconstruct the bi‐tangent in‐shader if desired
    float handedness =
        (glm::dot(glm::cross(normal, orth_tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;

    // Store the results back into the vertex
    vertex.tangent = glm::vec4(orth_tangent, handedness);
}

std::pair<glm::vec3, glm::vec3> Models::Model::calculate_tangent_bitangent(Models::Vertex v0,
                                                                           Models::Vertex v1,
                                                                           Models::Vertex v2) {

    glm::vec3 edge1 = v1.position - v0.position;
    glm::vec3 edge2 = v2.position - v0.position;
    glm::vec2 uv0   = v0.texcoord;
    glm::vec2 uv1   = v1.texcoord;
    glm::vec2 uv2   = v2.texcoord;

    glm::vec2 delta_uv1 = uv1 - uv0;
    glm::vec2 delta_uv2 = uv2 - uv0;
    // Compute the inverse of the determinant of the UV matrix (Δ)
    // This is equivalent to: Δ = 1 / (s1 * t2 - s2 * t1)
    float r = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);

    // Compute the tangent direction vector (T)
    // This solves: T = (t2 * Q1 - t1 * Q2) / Δ
    glm::vec3 tangent = {0.0f, 0.0f, 0.0f};
    // Compute the bitangent direction vector (B)
    // This solves: B = (-s2 * Q1 + s1 * Q2) / Δ
    glm::vec3 bitangent = {0.0f, 0.0f, 0.0f};

    tangent.x   = r * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
    tangent.y   = r * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
    tangent.z   = r * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);
    bitangent.x = r * (-delta_uv2.x * edge1.x + delta_uv1.x * edge2.x);
    bitangent.y = r * (-delta_uv2.x * edge1.y + delta_uv1.x * edge2.y);
    bitangent.z = r * (-delta_uv2.x * edge1.z + delta_uv1.x * edge2.z);

    return {tangent, bitangent};
}

void Models::Model::add_child(Model* child) {
    children.push_back(child);
}

void Models::Model::set_local_transform(const glm::mat4& local_transform) {
    model_instance.local_transform = local_transform;
}

void Models::Model::update_world_transform(const glm::mat4& parent_transform) {
    model_instance.world_transform = parent_transform * model_instance.local_transform;

    compute_aabb();
    for (Model* child : children) {
        child->update_world_transform(model_instance.world_transform);
    }
}

void Models::Model::draw(const glm::mat4& view, const glm::mat4& projection,
                         std::shared_ptr<Shader> shader) {

    // upload matrices
    shader->set_mat4("uView", view);
    shader->set_mat4("uProj", projection);
    shader->set_mat4("uModel", model_instance.world_transform);

    assert(model_data->vao != 0);
    GLCall(glBindVertexArray(model_data->vao));
    for (auto const& sm : model_data->submeshes) {
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
        }
        shader->set_bool("useAmbientMap", sm.mat.tex_Ka != 0 ? true : false);

        if (sm.mat.tex_Kd) {
            shader->set_texture("diffuseMap", sm.mat.tex_Kd, GL_TEXTURE2);
        }
        shader->set_bool("useDiffuseMap", sm.mat.tex_Kd != 0 ? true : false);

        if (sm.mat.tex_Ks) {
            shader->set_texture("specularMap", sm.mat.tex_Ks, GL_TEXTURE3);
        }
        shader->set_bool("useSpecularMap", sm.mat.tex_Ks != 0 ? true : false);

        if (sm.mat.tex_Bump) {
            shader->set_texture("bumpMap", sm.mat.tex_Bump, GL_TEXTURE4);
            shader->set_float("bumpScale", 4.0f);
        }
        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offsetPtr));
    }
    GLCall(glBindVertexArray(0));
}

void Models::Model::draw_depth(std::shared_ptr<Shader> shader) {

    shader->set_mat4("uModel", model_instance.world_transform);
    assert(model_data->vao != 0);

    GLCall(glBindVertexArray(model_data->vao));
    for (auto const& sm : model_data->submeshes) {
        void* offset_ptr = (void*)(sm.index_offset * sizeof(GLuint));
        GLCall(glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offset_ptr));
    }
    GLCall(glBindVertexArray(0));
}

void Models::Model::compute_aabb() {
    // 1) Initialize to extreme opposites
    glm::vec3 world_min(FLT_MAX);
    glm::vec3 world_max(-FLT_MAX);

    // 2) Transform each unique-vertex into world space and accumulate
    for (auto const& v : model_data->unique_vertices) {
        glm::vec4 wc = model_instance.world_transform * glm::vec4(v.position, 1.0f);
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
    model_instance.aabbmin = world_min;
    model_instance.aabbmax = world_max;
}

void Models::Model::compute_transformed_aabb(const glm::mat4& xf, glm::vec3& out_min,
                                             glm::vec3& out_max) {
    // all 8 corners of the local box
    glm::vec3 corners[8] = {
        {model_data->localaabbmin.x, model_data->localaabbmin.y, model_data->localaabbmin.z},
        {model_data->localaabbmin.x, model_data->localaabbmin.y, model_data->localaabbmax.z},
        {model_data->localaabbmin.x, model_data->localaabbmax.y, model_data->localaabbmin.z},
        {model_data->localaabbmin.x, model_data->localaabbmax.y, model_data->localaabbmax.z},

        {model_data->localaabbmax.x, model_data->localaabbmin.y, model_data->localaabbmin.z},
        {model_data->localaabbmax.x, model_data->localaabbmin.y, model_data->localaabbmax.z},
        {model_data->localaabbmax.x, model_data->localaabbmax.y, model_data->localaabbmin.z},
        {model_data->localaabbmax.x, model_data->localaabbmax.y, model_data->localaabbmax.z},
    };

    out_min = glm::vec3(FLT_MAX);
    out_max = glm::vec3(-FLT_MAX);

    for (auto& c : corners) {
        glm::vec3 w = glm::vec3(xf * glm::vec4(c, 1.0f));
        out_min     = glm::min(out_min, w);
        out_max     = glm::max(out_max, w);
    }

    const float eps = 0.001f;
    if (glm::epsilonEqual(out_min.y, out_max.y, glm::epsilon<float>())) {
        out_min.y -= eps;
        out_max.y += eps;
    }
}

float Models::Model::distance_from_point_using_AABB(const glm::vec3& point) {
    static const glm::vec3 convenience_offset{0.0f, -0.6f, 0.0f};
    glm::vec3              offset_cen = point + convenience_offset;
    glm::vec3 closest = glm::clamp(offset_cen, model_instance.aabbmin, model_instance.aabbmax);
    float     d2      = glm::length2(closest - offset_cen);
    return d2;
}

bool Models::Model::intersect_sphere_aabb(const glm::vec3& point, float radius) {
    auto squared_distance = distance_from_point_using_AABB(point);
    return squared_distance <= radius * radius;
}


bool Models::Model::aabb_in_frustum(const std::array<glm::vec4, 6>& P, const glm::vec3& minB,
                                    const glm::vec3& maxB) const {
    for (auto& plane : P) {
        // pick the “positive‐vertex” for this plane normal
        glm::vec3 n(plane);
        glm::vec3 positive = {n.x > 0.0f ? maxB.x : minB.x, n.y > 0.0f ? maxB.y : minB.y,
                              n.z > 0.0f ? maxB.z : minB.z};
        // if that vertex is outside, the whole box is outside
        if (glm::dot(n, positive) + plane.w < 0.0f) {
            return false;
        }
    }
    return true;
}

void Models::Model::in_frustum(const std::array<glm::vec4, 6>& frustum_planes) {
    // clear previous state
    model_instance.in_frustum = false;
    model_instance.in_frustum =
        aabb_in_frustum(frustum_planes, model_instance.aabbmin, model_instance.aabbmax);
    return;
}

//TODO create one function that does all of the walls in a simple way
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
    floor_material.Ka           = glm::vec3(0.15f, 0.07f, 0.02f); // dark ambient
    floor_material.Kd           = glm::vec3(0.59f, 0.29f, 0.00f); // brown diffuse
    floor_material.Ks           = glm::vec3(0.05f, 0.04f, 0.03f); // small specular
    floor_material.Ns           = 16.0f;                          // shininess
    floor_material.d            = 1.0f;                           // opacity
    floor_material.illum        = 2;                              // standard Phong
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
        {-roomSize, y0, z0}, // BL
        {+roomSize, y0, z0}, // BR
        {+roomSize, y1, z0}, // TR
        {-roomSize, y1, z0}  // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, -1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {0, 2, 1, 0, 3, 2};

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns    = 16.0f;
    wall_material.d     = 1.0f;
    wall_material.illum = 2;

    wall_material.use_bump_map = false;
    return Models::Model(wall_verts, wall_normals, wall_uvs, wall_indices, "WallFront",
                         wall_material);
}
Models::Model Models::createWallRight(float roomSize, float roomHeight) {
    // roomSize == half‐width & half‐depth of your room; roomHeight is the height of the wall.
    float z0 = roomSize;
    float y0 = 0.0f;
    float y1 = roomHeight;

    // bottom‐left, bottom‐right, top‐right, top‐left (CCW when viewed from -Z side)
    std::vector<glm::vec3> wall_verts = {
        {roomSize, y0, -z0}, // BL
        {roomSize, y0, z0},  // BR
        {roomSize, y1, z0},  // TR
        {roomSize, y1, -z0}  // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, -1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {0, 1, 2, 0, 2, 3};

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns    = 16.0f;
    wall_material.d     = 1.0f;
    wall_material.illum = 2;

    wall_material.use_bump_map = false;
    return Models::Model(wall_verts, wall_normals, wall_uvs, wall_indices, "WallRight",
                         wall_material);
}
Models::Model Models::createWallLeft(float roomSize, float roomHeight) {
    // roomSize == half‐width & half‐depth of your room; roomHeight is the height of the wall.
    float z0 = roomSize;
    float y0 = 0.0f;
    float y1 = roomHeight;

    // bottom‐left, bottom‐right, top‐right, top‐left (CCW when viewed from -Z side)
    std::vector<glm::vec3> wall_verts = {
        {-roomSize, y0, -z0}, // BL
        {-roomSize, y0, z0},  // BR
        {-roomSize, y1, z0},  // TR
        {-roomSize, y1, -z0}  // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, 1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {0, 2, 1, 0, 3, 2};

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka    = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd    = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks    = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns    = 16.0f;
    wall_material.d     = 1.0f;
    wall_material.illum = 2;

    wall_material.use_bump_map = false;
    return Models::Model(wall_verts, wall_normals, wall_uvs, wall_indices, "WallLeft",
                         wall_material);
}

Models::Model Models::createWallBack(float roomSize, float roomHeight) {
    // roomSize == half‐width & half‐depth of your room; roomHeight is the height of the wall.
    float z0 = -roomSize;
    float y0 = 0.0f;
    float y1 = roomHeight;

    // bottom‐left, bottom‐right, top‐right, top‐left (CCW when viewed from -Z side)
    std::vector<glm::vec3> wall_verts = {
        {-roomSize, y0, z0}, // BL
        {+roomSize, y0, z0}, // BR
        {+roomSize, y1, z0}, // TR
        {-roomSize, y1, z0}  // TL
    };

    // normal pointing *into* the room (= –Z)
    std::vector<glm::vec3> wall_normals(4, glm::vec3(0.0f, 0.0f, 1.0f));

    // standard UVs
    std::vector<glm::vec2> wall_uvs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // two triangles, wound CCW from the normal side
    std::vector<GLuint> wall_indices = {0, 1, 2, 0, 2, 3};

    // same material as your floor/ceiling (tweak as needed)
    Material wall_material;
    wall_material.Ka           = glm::vec3(0.15f, 0.07f, 0.02f);
    wall_material.Kd           = glm::vec3(0.59f, 0.29f, 0.00f);
    wall_material.Ks           = glm::vec3(0.05f, 0.04f, 0.03f);
    wall_material.Ns           = 16.0f;
    wall_material.d            = 1.0f;
    wall_material.illum        = 2;
    wall_material.use_bump_map = false;

    return Models::Model(wall_verts, wall_normals, wall_uvs, wall_indices, "WallBack",
                         wall_material);
}
