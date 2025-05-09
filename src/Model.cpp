#include "Model.h"

Model::~Model(){
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

void Model::add_child(Model* child){
    children.push_back(child);
}

void Model::set_local_transform(const glm::mat4& local_transform){
    this->local_transform = local_transform;
}

void Model::update_world_transform(const glm::mat4& parent_transform) {
    world_transform =  parent_transform * local_transform;

    for (Model* child : children) {
        child->update_world_transform(world_transform);
    }
}
