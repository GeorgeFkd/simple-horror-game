#pragma once

#include "OBJLoader.h"
#include "Shader.h"
#include "SubMesh.h"
#include <GL/glew.h>
#include <cfloat>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
namespace Models {

struct Vertex {
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;

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

class Model {
  public:
    void draw_depth(std::shared_ptr<Shader> shader) const;
    void draw_depth_instanced(std::shared_ptr<Shader> shader) const;
    void draw(const glm::mat4& view, const glm::mat4& projection,
              std::shared_ptr<Shader> shader) const;
        void set_local_transform(const glm::mat4& local_transform);
    void update_world_transform(const glm::mat4& parent_transform);
    void compute_aabb();
    void add_child(Model* child);
    void debug_dump() const;
    void move_relative_to(const glm::vec3& direction);

    inline bool intersectAABB(const glm::vec3& minA, const glm::vec3& maxA, const glm::vec3& minB,
                              const glm::vec3& maxB) {
        // If one box is completely to the “left” of the other, no collision
        if (maxA.x < minB.x || minA.x > maxB.x)
            return false;
        if (maxA.y < minB.y || minA.y > maxB.y)
            return false;
        if (maxA.z < minB.z || minA.z > maxB.z)
            return false;
        return true;
    }

    inline void set_local_transform(glm::mat4&& local_transform) {
        this->local_transform = std::move(local_transform);
    }

    inline glm::vec3 get_aabbmin() const {
        return aabbmin;
    }

    inline glm::vec3 get_aabbmax() const {
        return aabbmax;
    }

    inline bool is_instanced() const {
        return is_instanced_;
    }
    inline void set_interactivity(bool is_interactive) {
        this->interactable = is_interactive;
    }

    inline bool can_interact() {
        return interactable;
    }

    inline bool isActive() const {
        return this->active;
    }

    inline void toggleActive() {
        active = !active;
    }

    inline void disable() {
        this->active = false;
    }

    inline std::string instance_name(size_t i) {
        return label + instance_suffixes[i];
    }

    inline void enable() {
        this->active = true;
    }

    inline void set_scale(const glm::vec3& s) {
        local_transform = glm::scale(glm::mat4(1.0f), s) * local_transform;
    }

    inline void set_instance_transforms(const std::vector<glm::mat4> instance_transforms) {
        this->instance_transforms = instance_transforms;
    }

    const glm::vec3& get_instance_aabb_min(size_t i) const {
        return instance_aabb_min[i];
    }

    const glm::vec3& get_instance_aabb_max(size_t i) const {
        return instance_aabb_max[i];
    }

    inline void remove_instance_transform(const std::string& suffix) {
        auto it = std::find(instance_suffixes.begin(), instance_suffixes.end(), suffix);
        if (it == instance_suffixes.end()) {
            std::cerr << "Instance suffix not found: " << suffix << "\n";
            return;
        }

        size_t i = std::distance(instance_suffixes.begin(), it);

        // Remove the suffix and matching transform/AABBs
        instance_suffixes.erase(instance_suffixes.begin() + i);
        instance_transforms.erase(instance_transforms.begin() + i);
        instance_aabb_min.erase(instance_aabb_min.begin() + i);
        instance_aabb_max.erase(instance_aabb_max.begin() + i);

        std::cout << "After removal: " << instance_transforms.size() << "\n";
        // instance_transforms.erase(instance_transforms.begin() + i);
        // instance_aabb_max.erase(instance_aabb_max.begin() + i);
        // instance_aabb_min.erase(instance_aabb_min.begin() + i);
        // std::cout << "After removal: " << instance_transforms.size() << "\n";
    }

    inline size_t get_instance_count() const {
        return instance_transforms.size();
    }

    void add_instance_transform(const glm::mat4& transform,std::string suffix);
    void compute_transformed_aabb(const glm::mat4& xf, glm::vec3& out_min, glm::vec3& out_max);
    void init_instancing(size_t max_instances);
    void update_instance_data() const;

    Model(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals,
          const std::vector<glm::vec2>& texcoords, const std::vector<GLuint>& indices,
          std::string label, const Material& mat = Material());

    inline glm::mat4 get_world_transform() {
        return world_transform;
    }

    Model(const std::string& objFile, const std::string& label);

    inline const std::string& name() {
        return label;
    }

    ~Model();

  private:
    std::vector<Vertex>  unique_vertices;
    std::vector<SubMesh> submeshes;
    std::string          label;
    // where the model is located
    // relative to its parent
    glm::mat4 local_transform;
    // where the model is actually placed
    // in the world after applying all parent transforms
    glm::mat4              world_transform;

    std::vector<std::string> instance_suffixes;
    std::vector<glm::mat4> instance_transforms;
    std::vector<glm::vec3> instance_aabb_min;
    std::vector<glm::vec3> instance_aabb_max;
    void draw_instanced(const glm::mat4& view, const glm::mat4& projection,
                        std::shared_ptr<Shader> shader) const;

    GLuint instance_vbo  = 0;
    bool   is_instanced_ = false;
    GLuint vao, vbo, ebo = 0;
    GLuint texture_id = 0;

    // 1) Object-space AABB (min/max corners in mesh local coords)
    glm::vec3 localaabbmin;
    glm::vec3 localaabbmax;

    // 2) World-space AABB (after applying world_transform)
    glm::vec3 aabbmin;
    glm::vec3 aabbmax;

    bool                interactable = false;
    bool                active       = true;
    std::vector<Model*> children;
};
} // namespace Models
