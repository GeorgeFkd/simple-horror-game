#include "OBJLoader.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: obj_loader <path_to_obj_file>\n";
        return 1;
    }

    const std::string filename = argv[1];

    ObjectLoader::OBJLoader loader;
    loader.read_from_file(filename);
    loader.debug_dump();

    std::cout << "Finished loading.\n";
    // std::cout << "Vertices:        " << loader.m_vertices.size() << "\n";
    std::cout << "Vertices new:    " << loader.model_data.m_vertices.size() << "\n";
    // std::cout << "Normals:         " << loader.m_vertex_normals.size() << "\n";
    std::cout << "Normals new:     " << loader.model_data.m_vertex_normals.size() << "\n";
    // std::cout << "TexCoords:       " << loader.m_texture_coords.size() << "\n";
    std::cout << "TexCoords new:   " << loader.model_data.m_texture_coords.size() << "\n";
    // std::cout << "Faces:           " << loader.m_faces.size() << "\n";
    std::cout << "Faces new:       " << loader.model_data.m_faces.size() << "\n";


#ifdef DEBUG_OBJLOADER
    if (!loader.model_data.m_vertices.empty()) {
        std::cout << "First vertex: ";
        loader.print_glmvec4(loader.model_data.m_vertices.front());
    }
    if (!loader.model_data.m_vertex_normals.empty()) {
        std::cout << "First normal: ";
        loader.print_glmvec3(loader.model_data.m_vertex_normals.front());
    }
    if (!loader.model_data.m_texture_coords.empty()) {
        std::cout << "First texcoord: ";
        loader.print_glmvec2(loader.model_data.m_texture_coords.front());
    }
#endif

    return 0;
}
