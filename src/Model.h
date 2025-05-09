#pragma once

#include <glm.hpp>
#include <GL/glew.h>
#include <string> 
#include <vector>
#include <cfloat>

class Model {

    void draw(const glm::mat4& view, const glm::mat4& projection);
    void set_local_transform(const glm::mat4& local_transform);
    void update_world_transform(const glm::mat4& parent_transform);
    void compute_aabb();
    void add_child(Model* child);

    Model();
    ~Model();
private: 
    // where the model is located 
    // relative to its parent
    glm::mat4 local_transform; 
    // where the model is actually placed
    // in the world after applying all parent transforms
    glm::mat4 world_transform;

    GLuint vao, vbo, ebo; 
    GLuint texture_id; 
    GLuint shader_program; 
    GLsizei index_count;

    glm::vec3 aabbmin, aabbmax;

    std::vector<Model*> children; 
};