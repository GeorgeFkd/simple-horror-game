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
#include <limits>
#include <numeric>
#include "OBJLoader.h"
#include "SubMesh.h"
#include "Shader.h"

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
        void draw_depth(Shader* shader) const;
        void draw(const glm::mat4& view_projection, Shader* shader) const;
        void set_local_transform(const glm::mat4& local_transform);
        void update_world_transform(const glm::mat4& parent_transform);
        void compute_aabb();
        void add_child(Model* child);
        void debug_dump() const;

        inline bool intersectAABB(const glm::vec3& minA, const glm::vec3& maxA, const glm::vec3& minB, const glm::vec3& maxB){
            // If one box is completely to the “left” of the other, no collision
            if (maxA.x < minB.x || minA.x > maxB.x) return false;
            if (maxA.y < minB.y || minA.y > maxB.y) return false;
            if (maxA.z < minB.z || minA.z > maxB.z) return false;
            return true;
        }

        inline glm::vec3 get_aabbmin() const {
            return aabbmin;
        }

        inline glm::vec3 get_aabbmax() const{
            return aabbmax;
        }
        
        inline void set_scale(const glm::vec3& s) {
            local_transform = glm::scale(glm::mat4(1.0f), s) * local_transform;
        }
        inline std::string_view name() {
            return label;        
    }
        Model(const ObjectLoader::OBJLoader& loader,const std::string& label);
        ~Model();
    private: 
        std::vector<Vertex> unique_vertices;
        std::vector<SubMesh> submeshes;
        std::string_view label;
        // where the model is located 
        // relative to its parent
        glm::mat4 local_transform; 
        // where the model is actually placed
        // in the world after applying all parent transforms
        glm::mat4 world_transform;

        GLuint vao, vbo, ebo = 0; 
        GLuint texture_id = 0; 

        // 1) Object-space AABB (min/max corners in mesh local coords)
        glm::vec3 localaabbmin;
        glm::vec3 localaabbmax;

        // 2) World-space AABB (after applying world_transform)
        glm::vec3 aabbmin;
        glm::vec3 aabbmax;


        std::vector<Model*> children; 
    };
}
