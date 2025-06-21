#pragma once

#include "OBJLoader.h"
#include "Shader.h"
#include "SubMesh.h"
#include <GL/glew.h>
#include <cfloat>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/norm.hpp>
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


    using namespace GlHelpers;

    enum class InstanceModifiedTypes{
        NOT_MODIFIED,
        REMOVED,
    };

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

    class Model {
    public:
        void draw_depth(std::shared_ptr<Shader> shader);
        void draw_depth_instanced(std::shared_ptr<Shader> shader);
        void draw(const glm::mat4& view, const glm::mat4& projection,
                std::shared_ptr<Shader> shader);
        void draw_instanced(const glm::mat4& view, const glm::mat4& projection,
                std::shared_ptr<Shader> shader);
        void set_local_transform(const glm::mat4& local_transform);
        void update_world_transform(const glm::mat4& parent_transform);
        void compute_aabb();
        void add_child(Model* child);
        void debug_dump() const;
        void move_relative_to(const glm::vec3& direction);
        std::pair<glm::vec3, glm::vec3> calculate_tangent_bitangent(Models::Vertex v0, Models::Vertex v1, Models::Vertex v2);
        void orthogonalize_and_normalize_tb(Models::Vertex& vertex, const std::vector<glm::vec3>& accumulated_tangent, const std::vector<glm::vec3>& accumulated_bitangent, const size_t index);
        void add_instance_transform(const glm::mat4& transform,const std::string& suffix);
        void compute_transformed_aabb(const glm::mat4& xf, glm::vec3& out_min, glm::vec3& out_max);
        void init_instancing(size_t max_instances);
        void update_instance_data();
        void in_frustum(const std::array<glm::vec4,6>& P);
        bool aabb_in_frustum(const std::array<glm::vec4,6>& P, const glm::vec3& minB, const glm::vec3& maxB);

        std::tuple<std::string, bool, float> is_closer_than_current_model(const glm::vec3& point_to_check, float current_distance_from_closest_model);
        std::pair<bool, int> intersect_sphere_aabb(const glm::vec3& point, float radius);
        std::pair<float, int> distance_from_point_using_AABB(const glm::vec3& point);

        void remove_instance_transform(const std::string& suffix);

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

        inline glm::mat4 get_local_transform() {
            return local_transform;
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

        inline bool in_frustum() const{

            if(!is_instanced()){
                return inside_frustum_;
            }

            return std::any_of(instance_in_frustum.begin(), instance_in_frustum.end(), [](bool v){return v;});
        }

        inline bool can_interact() {
            return interactable;
        }

        inline bool is_active() const {
            return this->active;
        }

        inline void toggle_active() {
            active = !active;
        }

        inline void disable() {
            this->active = false;
        }

        inline void enable() {
            this->active = true;
        }

        inline void set_scale(const glm::vec3& s) {
            local_transform = glm::scale(glm::mat4(1.0f), s) * local_transform;
        }

        inline void set_instance_transforms(const std::vector<glm::mat4> instance_transforms) {
            this->instance_transforms = instance_transforms;
            instance_data_dirty = true;
        }

        const glm::vec3& get_instance_aabb_min(size_t i) const {
            return instance_aabb_min[i];
        }

        const glm::vec3& get_instance_aabb_max(size_t i) const {
            return instance_aabb_max[i];
        }

        inline size_t get_instance_count() const {
            return instance_transforms.size();
        }

        inline size_t get_active_instance_count() const {
            return std::count_if(
                instance_modifications.begin(),
                instance_modifications.end(),
                [](InstanceModifiedTypes m) {
                    return m != InstanceModifiedTypes::REMOVED;
                }
            );
        }

        inline glm::mat4 get_world_transform() const{
            return world_transform;
        }

        inline std::string name(std::size_t instance_index = 0) const {
            if (is_instanced() && instance_index >= 0 && instance_index < instance_suffixes.size()) {
                return label + instance_suffixes[instance_index];
            }
            return label;
        }

        Model(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals,
            const std::vector<glm::vec2>& texcoords, const std::vector<GLuint>& indices,
            std::string label, const Material& mat = Material());

        Model(const std::string& objFile, const std::string& label);

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
        std::vector<InstanceModifiedTypes> instance_modifications;
        std::vector<bool> instance_in_frustum;

        bool inside_frustum_ = true;
        bool instance_data_dirty = true;

        void draw_instanced(const glm::mat4& view, const glm::mat4& projection,
                            std::shared_ptr<Shader> shader) const;

        GLuint instance_vbo  = 0;
        bool   is_instanced_ = false;
        GLuint vao, vbo, ebo = 0;
        GLuint texture_id = 0;
        GLuint gl_instance_count = 0;

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
