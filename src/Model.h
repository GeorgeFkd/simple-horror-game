#pragma once
#include "OBJLoader.h"
#include <algorithm>
#include <cfloat>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>
#include "Vertex.h"
#include "MData.h"
namespace Models {

struct InstanceData {
    std::string label;
    glm::mat4   local_transform;
    glm::mat4   world_transform;
    glm::vec3   aabbmin;
    glm::vec3   aabbmax;
    bool        in_frustum;
    bool        interactable = false;
    bool        active = true;
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
    InstanceData model_instance;
    std::shared_ptr<MData>                                                model_data;
    void  set_local_transform(const glm::mat4& local_transform);
    void  update_world_transform(const glm::mat4& parent_transform);
    void  add_child(Model* child);
    void  debug_dump() const;
    void  move_relative_to(const glm::vec3& direction);
    float distance_from_point_using_AABB(const glm::vec3& point);

    void in_frustum(const std::array<glm::vec4, 6>& P);

    bool intersect_sphere_aabb(const glm::vec3& point, float radius);

    inline void set_local_transform(glm::mat4&& local_transform) {
        model_instance.local_transform = std::move(local_transform);
    }

    inline glm::mat4 get_local_transform() {
        return model_instance.local_transform;
    }

    inline glm::vec3 get_aabbmin() const {
        return model_instance.aabbmin;
    }

    inline glm::vec3 get_aabbmax() const {
        return model_instance.aabbmax;
    }

    inline void set_interactivity(bool is_interactive) {
        this->model_instance.interactable = is_interactive;
    }

    inline bool is_in_frustum() const {
        return model_instance.in_frustum;
    }

    inline bool can_interact() {
        return model_instance.interactable;
    }

    inline bool is_active() const {
        return this->model_instance.active;
    }

    inline void toggle_active() {
        model_instance.active = !model_instance.active;
    }

    inline void disable() {
        this->model_instance.active = false;
    }

    inline void enable() {
        this->model_instance.active = true;
    }

    inline void set_scale(const glm::vec3& s) {
        model_instance.local_transform =
            glm::scale(glm::mat4(1.0f), s) * model_instance.local_transform;
    }

    inline glm::mat4 get_world_transform() const {
        return model_instance.world_transform;
    }

    inline std::string name() const {
        return model_instance.label;
    }

    Model(const Model& model_to_replicate, std::string name, glm::mat4 transform);

    Model(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals,
          const std::vector<glm::vec2>& texcoords, const std::vector<GLuint>& indices,
          std::string label, const Material& mat = Material());

    Model(const std::string& objFile, std::string label);

  private:
    // this is made not for caching, but for sharing MData between instances coming from the same
    // model, this saves opengl allocations and the computation to go from raw vertices to usable data(which is generally cheap)
    inline static std::unordered_map<std::string, std::shared_ptr<MData>> model_registry;
    
    void compute_transformed_aabb(const glm::mat4& xf, glm::vec3& out_min, glm::vec3& out_max);
    bool aabb_in_frustum(const std::array<glm::vec4, 6>& P, const glm::vec3& minB,
                         const glm::vec3& maxB) const;

    std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> prepare_bitangents();
    std::pair<glm::vec3, glm::vec3>
    calculate_tangent_bitangent(Vertex v0, Vertex v1, Vertex v2);

    void initialize_local_aabb();
    void update_aabb();
    void orthogonalize_and_normalize_tb(Vertex&               vertex,
                                        const std::vector<glm::vec3>& accumulated_tangent,
                                        const std::vector<glm::vec3>& accumulated_bitangent,
                                        const size_t                  index);

    
    //these 2 could also be on the model_instance 
    // bool interactable = false;
    // bool active       = true;
    // should be part of model data probably
    std::vector<Model*> children;
};

Model createWallFront(float roomSize, float roomHeight);
Model createWallBack(float roomSize, float roomHeight);
Model createWallLeft(float roomSize, float roomHeight);
Model createWallRight(float roomSize, float roomHeight);
Model createCeiling(float roomSize, float roomHeight);
Model createFloor(float roomSize);

} // namespace Models
