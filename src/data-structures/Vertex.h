#pragma once
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <unordered_map>
struct Vertex {
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
    glm::vec4 tangent;

    bool operator==(Vertex const& o) const {
        return glm::all(glm::epsilonEqual(position, o.position, glm::epsilon<float>())) &&
               glm::all(glm::epsilonEqual(texcoord, o.texcoord, glm::epsilon<float>())) &&
               glm::all(glm::epsilonEqual(normal, o.normal, glm::epsilon<float>()));
    }
};

struct VertexHasher {
    // TODO I don't know if thats good enough
    size_t operator()(Vertex const& v) const noexcept {
        auto h = std::hash<float>{};
        // xor mixes the bits together
        // while shifting them reduces collisions so they can be
        // distributed evenly across buckets
        size_t h0 = h(v.position.x) ^ (h(v.position.y) << 1) ^ (h(v.position.z) << 2);
        size_t h1 = h(v.texcoord.x) ^ (h(v.texcoord.y) << 1);
        size_t h2 = h(v.normal.x) ^ (h(v.normal.y) << 1) ^ (h(v.normal.z) << 2);
        return h0 ^ (h1 << 1) ^ (h2 << 2);
    }
};
