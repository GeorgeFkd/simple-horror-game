#include "ModelLoader.h"
#include "OBJLoader.h"
std::shared_ptr<MData> ModelLoader::load_or_get_cached(const std::string& objFile) {
    auto it = cache.find(objFile);
    if (it != cache.end()) {
        return it->second;
    }

    // Load and cache
    std::shared_ptr<MData> data = ModelLoader::parse_from_obj_file(objFile);
    cache.emplace(objFile, data);
    return data;
}


std::shared_ptr<MData> ModelLoader::parse_from_obj_file(const std::string& objFile) {
    ObjectLoader::OBJLoader loader;
    auto                    obj_model_data = loader.read_from_file(objFile);

    auto model_data = std::make_shared<MData>();
    // build unique_vertices & a cache
    std::unordered_map<Vertex, GLuint, VertexHasher> cache;
    cache.reserve(obj_model_data->m_faces.size() * 4);

    // bucket indices by material_id
    std::unordered_map<int, std::vector<GLuint>> buckets;

    auto default_texcoord = glm::vec2(0.0f, 0.0f);
    auto default_normal   = glm::vec3(0.0f, 0.0f, 1.0f);
    auto add_vertex       = [&](int vi, int ti, int ni) {
        Vertex vert;
        vert.position = glm::vec3(obj_model_data->m_vertices[vi]);
        if (ti >= 0 && ti < (int)obj_model_data->m_texture_coords.size()) {
            vert.texcoord = obj_model_data->m_texture_coords[ti];
        } else {
            vert.texcoord = default_texcoord;
        }

        if (ni >= 0 && ni < (int)obj_model_data->m_vertex_normals.size()) {
            vert.normal = obj_model_data->m_vertex_normals[ni];
        } else {
            vert.normal = default_normal;
        }

        auto [it, inserted] = cache.emplace(vert, (GLuint)model_data->unique_vertices.size());
        if (inserted) {
            model_data->unique_vertices.push_back(vert);
        }
        return it->second;
    };

    for (auto const& face : obj_model_data->m_faces) {
        int material_id = face.material_id;
        // unpack up to 4 verts; 3 if w == -1
        int vertex_count = (face.vertices.w == -1 ? 3 : 4);
        // first tri
        buckets[material_id].push_back(
            add_vertex(face.vertices[0], face.texcoords[0], face.normals[0]));
        buckets[material_id].push_back(
            add_vertex(face.vertices[1], face.texcoords[1], face.normals[1]));
        buckets[material_id].push_back(
            add_vertex(face.vertices[2], face.texcoords[2], face.normals[2]));
        // second tri if quad
        if (vertex_count == 4) {
            buckets[material_id].push_back(
                add_vertex(face.vertices[0], face.texcoords[0], face.normals[0]));
            buckets[material_id].push_back(
                add_vertex(face.vertices[2], face.texcoords[2], face.normals[2]));
            buckets[material_id].push_back(
                add_vertex(face.vertices[3], face.texcoords[3], face.normals[3]));
        }
    }

    // flatten buckets → one big index array, record submeshes
    model_data->indices.reserve(
        std::accumulate(buckets.begin(), buckets.end(), 0u,
                        [](auto sum, auto& p) { return sum + p.second.size(); }));

    for (auto& [material_id, indexes] : buckets) {
        SubMesh sm;
        if (material_id >= 0) {
            sm.mat = obj_model_data->m_materials[material_id];
        } else {
            sm.mat = Material{};
        }
        sm.index_offset = (GLuint)model_data->indices.size();
        sm.index_count  = (GLuint)indexes.size();

        model_data->indices.insert(model_data->indices.end(), indexes.begin(), indexes.end());
        model_data->submeshes.push_back(sm);
    }

    model_data->initialize();
    // storage for accumulating each shared vertex's contributions
    // auto [tan1, tan2] = prepare_bitangents(model_data.get());
    // for (int i = 0; i < model_data->unique_vertices.size(); i++) {
    //     orthogonalize_and_normalize_tb(model_data->unique_vertices[i], tan1, tan2, i);
    // }
    //
    // initialize_local_aabb(model_data.get());
    return model_data;
}
