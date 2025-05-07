#include "Renderer.h"


void Renderer::RendererObj::load_model(const ObjectLoader::OBJLoader& loader){

    std::vector<Renderer::Vertex> unique_vertices;
    std::vector<GLuint> indices;


    unique_vertices.reserve(loader.m_faces.size() * 6);
    indices.reserve(loader.m_faces.size() * 6);


    for (auto const& face: loader.m_faces){

        int v[4] = {face.vertices[0], face.vertices[1], face.vertices[2], face.vertices[3]};
        int t[4] = {face.texcoords[0], face.texcoords[1], face.texcoords[2], face.texcoords[3]};
        int n[4] = {face.normals[0], face.normals[1], face.normals[2], face.normals[3]};


        auto add_vertex = [&](int vi, int ti, int ni){

            Vertex vertex;

            // drop 4D coordinates 
            vertex.position = glm::vec3(loader.m_vertices[vi]);
            vertex.texcoord = glm::vec2(loader.m_texture_coords[ti]);
            vertex.normal = loader.m_vertex_normals[ni];

            auto it = std::find(unique_vertices.begin(), unique_vertices.end(), vertex);

            if(it == unique_vertices.end()){
                unique_vertices.push_back(vertex);
                indices.push_back(static_cast<GLuint>(unique_vertices.size() - 1));
            }else{
                indices.push_back(static_cast<GLuint>(std::distance(unique_vertices.begin(), it)));
            }
        };

        // Create the two triangles from the Quad
        // if its not a Quad
        // Tri 1: v0, v1, v2
        add_vertex(v[0], t[0], n[0]);
        add_vertex(v[1], t[1], n[1]);
        add_vertex(v[2], t[2], n[2]);

        // Tri 2: v0, v2, v3
        if (face.vertices[3] != -1) { // quad
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

//void Renderer::RendererObj::render(){
//
//}