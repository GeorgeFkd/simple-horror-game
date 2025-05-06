#include "Renderer.h"


void Renderer::RendererObj::load_model(const ObjectLoader::OBJLoader& loader){

    std::vector<Renderer::Vertex> unique_vertices;
    std::vector<GLuint> indices;


    unique_vertices.reserve(loader.m_faces.size() * 6);
    indices.reserve(loader.m_faces.size() * 6);


    for (auto const& face: loader.m_faces){
        int v[4] = {};
        int t[4] = {};

    }
}

