#include "loaders/OBJLoader.h"
#include <gtest/gtest.h>


TEST(ObjLoader, BasicAssertions) {

    ObjectLoader::OBJLoader loader;
    // i want a way to not have to specify it like this
    const auto* filename = "./assets/test.obj";
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

}

