#include "Model.h"

void Model::Model::debug_dump(){
    std::cout 
    << "  >>> Model built: "
    << unique_vertices.size() << " unique vertices, "
    << (indices.size()/3)       << " triangles, "
    << indices.size()           << " indices total.\n";
    std::cout 
    << "      AABB local min = (" 
    << localAABBMin.x << ","
    << localAABBMin.y << ","
    << localAABBMin.z << ")\n"
    << "      AABB local max = (" 
    << localAABBMax.x << ","
    << localAABBMax.y << ","
    << localAABBMax.z << ")\n";
}

Model::Model::Model(const ObjectLoader::OBJLoader& loader){
    
    // maps each unique Vertex → its index in unique_vertices
    std::unordered_map<Vertex, GLuint, VertexHasher> cache;
    cache.reserve(loader.m_faces.size()*4);

    // Build triangles...
    for (auto const& face : loader.m_faces) {
      int v[4] = { face.vertices[0], face.vertices[1],
                   face.vertices[2], face.vertices[3] };
      int t[4] = { face.texcoords[0], face.texcoords[1],
                   face.texcoords[2], face.texcoords[3] };
      int n[4] = { face.normals[0], face.normals[1],
                   face.normals[2], face.normals[3] };

      auto add_vertex = [&](int vi, int ti, int ni){
        Vertex vert;
        vert.position = glm::vec3(loader.m_vertices[vi]);
        vert.texcoord = glm::vec2(loader.m_texture_coords[ti]);
        vert.normal   = loader.m_vertex_normals[ni];

        auto it_and_ins = cache.emplace(vert, (GLuint)unique_vertices.size());
        if (it_and_ins.second) {
          // was newly inserted
          unique_vertices.push_back(vert);
        }
        // either way, push the (existing or new) index:
        indices.push_back(it_and_ins.first->second);
      };

      // first triangle
      add_vertex(v[0], t[0], n[0]);
      add_vertex(v[1], t[1], n[1]);
      add_vertex(v[2], t[2], n[2]);

      // second triangle (if quad)
      if (v[3] != -1) {
        add_vertex(v[0], t[0], n[0]);
        add_vertex(v[2], t[2], n[2]);
        add_vertex(v[3], t[3], n[3]);
      }
    }

    index_count = static_cast<GLsizei>(indices.size());

    localAABBMin = glm::vec3(FLT_MAX);
    localAABBMax = glm::vec3(-FLT_MAX);
    for (const auto& v : unique_vertices) {
        localAABBMin = glm::min(localAABBMin, glm::vec3(v.position));
        localAABBMax = glm::max(localAABBMax, glm::vec3(v.position));
    }

    // generate VAO/VBO/EBO if needed
    // VAO groups the vertex attribute setup
    // VBO stores vertex data (positions, texcoords, normals)
    // EBO stores indices for indexed drawing
    if (vao == 0) glGenVertexArrays(1, &vao);
    if (vbo == 0) glGenBuffers(1, &vbo);
    if (ebo == 0) glGenBuffers(1, &ebo);

    // bind & upload
    glBindVertexArray(vao);

    // upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 unique_vertices.size() * sizeof(Vertex),
                 unique_vertices.data(),
                 GL_STATIC_DRAW);

    // upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(GLuint),
                 indices.data(),
                 GL_STATIC_DRAW);

    // attribute setup
    // position : layout(location = 0) in vec3
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, position));

    // texcoord : layout(location = 1) in vec2
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, texcoord));

    // normal   : layout(location = 2) in vec3
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));

    index_count = static_cast<GLsizei>(indices.size());
    // unbind VAO (the ELEMENT_ARRAY_BUFFER binding sticks with the VAO)
    glBindVertexArray(0);
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
    if (shader_program) {
        glDeleteProgram(shader_program);
        shader_program = 0;
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

    if (shader_program==0){
        return;
    }
    // use the shader program
    glUseProgram(shader_program);

    // upload the combined view-projection matrix
    // note the shader needs to have the uViewProj defined
    GLint loc_view_projection = glGetUniformLocation(shader_program, "uViewProj");
    glUniformMatrix4fv(loc_view_projection, 1, GL_FALSE, glm::value_ptr(view_projection));

    GLint loc_model = glGetUniformLocation(shader_program, "uModel");
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, glm::value_ptr(world_transform));

    // bind the VAO (which already has the VBO/EBO & attrib pointers from load_model)
    glBindVertexArray(vao);

    // draw all indices as triangles
    glDrawElements(
        GL_TRIANGLES,       // we're drawing triangles
        index_count,        // number of indices in the EBO
        GL_UNSIGNED_INT,    // the type of the indices
        nullptr             // offset into the EBO (0 here)
    );

    // unbind to avoid accidental state leakage
    glBindVertexArray(0);
    glUseProgram(0);
}


void Model::Model::compute_aabb() {
    // build 8 corners from the **object-space** box
    glm::vec3 corners[8] = {
        {localAABBMin.x, localAABBMin.y, localAABBMin.z},
        {localAABBMax.x, localAABBMin.y, localAABBMin.z},
        {localAABBMin.x, localAABBMax.y, localAABBMin.z},
        {localAABBMax.x, localAABBMax.y, localAABBMin.z},
        {localAABBMin.x, localAABBMin.y, localAABBMax.z},
        {localAABBMax.x, localAABBMin.y, localAABBMax.z},
        {localAABBMin.x, localAABBMax.y, localAABBMax.z},
        {localAABBMax.x, localAABBMax.y, localAABBMax.z}
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

