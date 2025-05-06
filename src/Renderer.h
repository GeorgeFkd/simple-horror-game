#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include "OBJLoader.h"

namespace Renderer{

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

    class RendererObj{
    public: 

        void load_model(const ObjectLoader::OBJLoader& loader);

        RendererObj(int width, int height);
        ~RendererObj();
    private:
        GLuint vao, vbo, ebo;
        GLuint shader_program;
        int screen_width, screen_height;

        
        GLuint load_shader(const std::string& vertPath, const std::string& fragPath);
        std::string load_file(const std::string& path);
    };

}