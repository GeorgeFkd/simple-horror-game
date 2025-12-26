#include "GPUMesh.h"
#include "Shader.h"
GPUMesh::~GPUMesh() {
    deleteBuffer(&ebo);
    deleteBuffer(&vbo);
    deleteVertexArray(&vao);
}

GPUMesh::GPUMesh() {

}

void GPUMesh::reserve_opengl_memory(MData* model_data) {
    makeVertexArray(&vao);
    makeBuffer(&vbo);
    makeBuffer(&ebo);

    bindVAO(vao);
    bindBuffer(vbo);
    bindBufferData(model_data->unique_vertices.size() * sizeof(Vertex), (void*) model_data->unique_vertices.data());
    bindElementBuffer(ebo);
    bindElementBufferData(model_data->indices.size() * sizeof(GLuint), (void*) model_data->indices.data());

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
