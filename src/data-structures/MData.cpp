#include "Shader.h"
#include "MData.h"
MData::~MData() {
    deleteBuffer(&ebo);
    deleteBuffer(&vbo);
    deleteVertexArray(&vao);
}

void MData::reserve_open_gl_memory() {
    makeVertexArray(&vao);
    makeBuffer(&vbo);
    makeBuffer(&ebo);

    bindVAO(vao);
    bindBuffer(vbo);
    bindBufferData(unique_vertices.size() * sizeof(Vertex), unique_vertices.data());
    bindElementBuffer(ebo);
    bindElementBufferData(indices.size() * sizeof(GLuint), indices.data());

    auto size = sizeof(Vertex);
    enableVAttribArray(0);
    bindVAttribPointer(0, 3, size, (void*)offsetof(Vertex, position));
    enableVAttribArray(1);
    bindVAttribPointer(1, 2, size, (void*)offsetof(Vertex, texcoord));
    enableVAttribArray(2);
    bindVAttribPointer(2, 3, size, (void*)offsetof(Vertex, normal));
    enableVAttribArray(3);
    bindVAttribPointer(3, 4, size, (void*)offsetof(Vertex, tangent));

    unbindVAO();
}
