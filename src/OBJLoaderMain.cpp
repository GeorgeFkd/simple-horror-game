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

    std::cout << "Finished loading.\n";
    std::cout << "Vertices:        " << loader.m_vertices.size() << "\n";
    std::cout << "Normals:         " << loader.m_vertex_normals.size() << "\n";
    std::cout << "TexCoords:       " << loader.m_texture_coords.size() << "\n";
    std::cout << "Faces:           " << loader.m_faces.size() << "\n";

#ifdef DEBUG_OBJLOADER
    if (!loader.m_vertices.empty()) {
        std::cout << "First vertex: ";
        loader.print_glmvec4(loader.m_vertices.front());
    }
    if (!loader.m_vertex_normals.empty()) {
        std::cout << "First normal: ";
        loader.print_glmvec3(loader.m_vertex_normals.front());
    }
    if (!loader.m_texture_coords.empty()) {
        std::cout << "First texcoord: ";
        loader.print_glmvec2(loader.m_texture_coords.front());
    }
#endif

    return 0;
}
