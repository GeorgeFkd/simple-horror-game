#include "EntityID.h"
#include "MData.h"
#include <vector>
MData::~MData() {}


MData::MData() {
    id = next_entity_id();
}


MData::MData(std::vector<GLuint>&& indices,
      std::vector<Vertex>&& unique_vertices,
      std::vector<SubMesh>&& submeshes)
    : indices(std::move(indices)),
      unique_vertices(std::move(unique_vertices)),
      submeshes(std::move(submeshes)) 
{
    id = next_entity_id();
}





void MData::MData::initialize() {
        auto [tan1, tan2] = prepare_bitangents();
    for (int i = 0; i < unique_vertices.size(); i++) {
        orthogonalize_and_normalize_tb(unique_vertices[i], tan1, tan2, i);
    }

    initialize_local_aabb();

}


void MData::initialize_local_aabb() {
    localaabbmin = glm::vec3(std::numeric_limits<float>::max());
    localaabbmax = glm::vec3(-std::numeric_limits<float>::max());
    for (auto const& v : unique_vertices) {
        localaabbmin = glm::min(localaabbmin, v.position);
        localaabbmax = glm::max(localaabbmax, v.position);
    }
}


void MData::orthogonalize_and_normalize_tb(
    Vertex& vertex, const std::vector<glm::vec3>& accumulated_tangent,
    const std::vector<glm::vec3>& accumulated_bitangent, const size_t index) {
    const glm::vec3& normal    = vertex.normal;
    const glm::vec3& tangent   = accumulated_tangent[index];
    const glm::vec3& bitangent = accumulated_bitangent[index];

    //
    // Gram–Schmidt orthogonalize the tangent against the normal
    glm::vec3 orth_tangent = glm::normalize(tangent - normal * glm::dot(normal, tangent));

    // Compute handedness (±1) so we can reconstruct the bi‐tangent in‐shader if desired
    float handedness =
        (glm::dot(glm::cross(normal, orth_tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;

    // Store the results back into the vertex
    vertex.tangent = glm::vec4(orth_tangent, handedness);
}

static std::pair<glm::vec3, glm::vec3> calculate_tangent_bitangent(Vertex v0, Vertex v1,
                                                                           Vertex v2) {

    glm::vec3 edge1 = v1.position - v0.position;
    glm::vec3 edge2 = v2.position - v0.position;
    glm::vec2 uv0   = v0.texcoord;
    glm::vec2 uv1   = v1.texcoord;
    glm::vec2 uv2   = v2.texcoord;

    glm::vec2 delta_uv1 = uv1 - uv0;
    glm::vec2 delta_uv2 = uv2 - uv0;
    // Compute the inverse of the determinant of the UV matrix (Δ)
    // This is equivalent to: Δ = 1 / (s1 * t2 - s2 * t1)
    float r = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);

    // Compute the tangent direction vector (T)
    // This solves: T = (t2 * Q1 - t1 * Q2) / Δ
    glm::vec3 tangent = {0.0f, 0.0f, 0.0f};
    // Compute the bitangent direction vector (B)
    // This solves: B = (-s2 * Q1 + s1 * Q2) / Δ
    glm::vec3 bitangent = {0.0f, 0.0f, 0.0f};

    tangent.x   = r * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
    tangent.y   = r * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
    tangent.z   = r * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);
    bitangent.x = r * (-delta_uv2.x * edge1.x + delta_uv1.x * edge2.x);
    bitangent.y = r * (-delta_uv2.x * edge1.y + delta_uv1.x * edge2.y);
    bitangent.z = r * (-delta_uv2.x * edge1.z + delta_uv1.x * edge2.z);

    return {tangent, bitangent};
}


std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> MData::prepare_bitangents() {
    std::vector<glm::vec3> tan1(unique_vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(unique_vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        GLuint i0 = indices[i + 0];
        GLuint i1 = indices[i + 1];
        GLuint i2 = indices[i + 2];

        const auto& v0 = unique_vertices[i0];
        const auto& v1 = unique_vertices[i1];
        const auto& v2 = unique_vertices[i2];

        auto [T, B] = calculate_tangent_bitangent(v0, v1, v2);

        tan1[i0] += T;
        tan1[i1] += T;
        tan1[i2] += T;
        tan2[i0] += B;
        tan2[i1] += B;
        tan2[i2] += B;
    }
    return std::make_pair(tan1, tan2);
}




