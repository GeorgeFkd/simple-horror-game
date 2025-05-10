#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string> 
#include <vector>
#include <cfloat>
#include <unordered_map>
#include "OBJLoader.h"


namespace Model{

    struct Vertex {
        glm::vec3 position;
        glm::vec2 texcoord;
        glm::vec3 normal;

        bool operator==(Vertex const& o) const {
            return glm::all(glm::epsilonEqual(position, o.position, glm::epsilon<float>()))
            && glm::all(glm::epsilonEqual(texcoord, o.texcoord, glm::epsilon<float>()))
            && glm::all(glm::epsilonEqual(normal, o.normal, glm::epsilon<float>()));
        }
    };

    struct VertexHasher {
        //TODO I don't know if thats good enough
        size_t operator()(Vertex const& v) const noexcept{
            auto h = std::hash<float>{};
            // xor mixes the bits together 
            // while shifting them reduces collisions so they can be 
            // distributed evenly across buckets
            size_t h0 = h(v.position.x) ^ (h(v.position.y) << 1) ^ (h(v.position.z) << 2);
            size_t h1 = h(v.texcoord.x) ^ (h(v.texcoord.y) << 1);
            size_t h2 = h(v.normal.x)   ^ (h(v.normal.y) << 1) ^ (h(v.normal.z) << 2);
            return h0 ^ (h1 << 1) ^ (h2 << 2);
        }
    };



    class Model {
    public:
        void draw(const glm::mat4& view_projection);
        void set_local_transform(const glm::mat4& local_transform);
        void update_world_transform(const glm::mat4& parent_transform);
        void compute_aabb();
        void add_child(Model* child);
        void debug_dump();

        void set_shader_program(GLuint shader_program);

        Model(const ObjectLoader::OBJLoader& loader);
        ~Model();
    private: 
        std::vector<Vertex> unique_vertices;
        std::vector<GLuint> indices;
        // where the model is located 
        // relative to its parent
        glm::mat4 local_transform; 
        // where the model is actually placed
        // in the world after applying all parent transforms
        glm::mat4 world_transform;

        GLuint vao, vbo, ebo = 0; 
        GLuint texture_id = 0; 
        GLuint shader_program = 0; 
        GLsizei index_count = 0;

        // 1) Object-space AABB (min/max corners in mesh local coords)
        glm::vec3 localAABBMin;
        glm::vec3 localAABBMax;

        // 2) World-space AABB (after applying world_transform)
        glm::vec3 aabbmin;
        glm::vec3 aabbmax;


        std::vector<Model*> children; 
    };
}