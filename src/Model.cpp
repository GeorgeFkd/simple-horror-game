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


void Model::compute_aabb() {
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

