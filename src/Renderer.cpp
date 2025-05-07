#include "Renderer.h"


void Renderer::RendererObj::load_model(const ObjectLoader::OBJLoader& loader){

    std::vector<Vertex> unique_vertices;
    std::vector<GLuint> indices;

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


std::string Renderer::RendererObj::load_file(const std::string& path){
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Failed to open file: " + path);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Renderer::RendererObj::compile_shader(GLenum type, const std::string& source){
    // Every symbolic constant you pass to an OpenGL 
    // function—like GL_ARRAY_BUFFER, GL_TRIANGLES, GL_FLOAT, 
    // GL_BLEND, etc.—is actually just an integer constant 
    // of type GLenum.
    // glBindBuffer((GLenum)0x8892, vbo);
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if(success == GL_FALSE){
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(max_length);
        glGetShaderInfoLog(shader, max_length, &max_length, &errorLog[0]);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        return -1;
    }

    return shader;
}

//void Renderer::RendererObj::render(){
//
//}