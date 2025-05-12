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
         << localaabbmax.z << ")\n"
      << "      Shader program ID: "
      << shader_program << "\n";
}

Model::Model::Model(const ObjectLoader::OBJLoader& loader)
  : local_transform(1.0f)
  , world_transform(1.0f)
  , localaabbmin(std::numeric_limits<float>::max())
  , localaabbmax(-std::numeric_limits<float>::max())
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

void Model::Model::set_shader_program(GLuint shader_program){
    this->shader_program = shader_program;
};

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

void Model::Model::draw(const glm::mat4& view_projection){
    glBindVertexArray(vao);

    // upload matrices
    GLint locVP = glGetUniformLocation(shader_program, "uViewProj");
    GLint locM  = glGetUniformLocation(shader_program, "uModel");
    glUniformMatrix4fv(locVP, 1, GL_FALSE, glm::value_ptr(view_projection));
    glUniformMatrix4fv(locM,  1, GL_FALSE, glm::value_ptr(world_transform));

    // helper that logs if the uniform isn't active
    auto checkUniform = [&](const char* name) {
        GLint loc = glGetUniformLocation(shader_program, name);
        if (loc < 0) {
            std::cerr << "Warning: uniform `" << name << "` not found.\n";
        }        
        return loc;
    };

    // *before* submesh loop, look up & check each uniform once:
    GLint locKa       = checkUniform("material.ambient");
    GLint locKd       = checkUniform("material.diffuse");
    GLint locKs       = checkUniform("material.specular");
    GLint locKe       = checkUniform("material.emissive");
    GLint locNs       = checkUniform("material.shininess");
    GLint locOpacity  = checkUniform("material.opacity");
    GLint locIllum    = checkUniform("material.illumModel");
    GLint locIor      = checkUniform("material.ior");

    // now draw each submesh, reusing the locations:
    for (auto const& sm : submeshes) {
        if (locKa      >= 0) glUniform3fv(locKa,      1, glm::value_ptr(sm.mat.Ka));
        if (locKd      >= 0) glUniform3fv(locKd,      1, glm::value_ptr(sm.mat.Kd));
        if (locKs      >= 0) glUniform3fv(locKs,      1, glm::value_ptr(sm.mat.Ks));
        if (locKe      >= 0) glUniform3fv(locKe,      1, glm::value_ptr(sm.mat.Ke));
        if (locNs      >= 0) glUniform1f (locNs,       sm.mat.Ns);
        if (locOpacity >= 0) glUniform1f (locOpacity,  sm.mat.d);
        if (locIllum   >= 0) glUniform1i (locIllum,    sm.mat.illum);
        if (locIor     >= 0) glUniform1f (locIor,      sm.mat.Ni);

        // draw elements…
        void* offsetPtr = (void*)(sm.index_offset * sizeof(GLuint));
        glDrawElements(GL_TRIANGLES,
                        sm.index_count,
                        GL_UNSIGNED_INT,
                        offsetPtr);
    }

    glBindVertexArray(0);
}


void Model::Model::draw_depth(GLuint depth_shader){
    glUseProgram(depth_shader);
    GLint locM = glGetUniformLocation(depth_shader, "uModel");
    glUniformMatrix4fv(locM, 1, GL_FALSE, glm::value_ptr(world_transform));

    glBindVertexArray(vao);
    for (auto const& sm : submeshes) {
        void* offset = (void*)(sm.index_offset * sizeof(GLuint));
        glDrawElements(GL_TRIANGLES, sm.index_count, GL_UNSIGNED_INT, offset);
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

