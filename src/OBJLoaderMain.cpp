#include <iostream>
#include "OBJLoader.h"


int main(int argc, char **argv) {
    std::cout << "Hello world\n";
    if(argc < 2){
        std::cout << "usage: <executable> filename.obj\n";
        exit(1);
    }
    std::string filename = argv[1];

    OBJLoader loader;
    loader.read_from_file(filename);

    std::cout << "Model has: " << loader.m_vertices.size() << " vertices\n";
    std::cout << "Model has: " << loader.m_vertex_normals.size() << " vertex normals\n";
    std::cout << "Model has: " << loader.m_texture_coords.size() << " texture coords\n";
    std::cout << "Model has: " << loader.m_faces.size() << " faces\n";
}
