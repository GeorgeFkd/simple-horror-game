#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
//should probably use data structures from the libraries



class OBJLoader{
    

private: 
    void read_vertex(const char* buff);
    void read_texcoord(const char* buff);
    void read_normal(const char* buff);
    void read_faceLimited(const char* buff);
    void read_usemtl(const char* buff,int &materialID);
    void read_mtllib(const char* buff,const std::string& filename);
    void add_new_group(const char* buff,int &materialID);


public:
    OBJLoader();
    ~OBJLoader();
    void read_from_file(const std::string& filename);
    std::vector<glm::vec4>m_vertices;
    std::vector<glm::vec3> m_texture_coords;  
    std::vector<glm::vec3>m_vertex_normals;

    struct Face {
        glm::ivec4 vertices;
        glm::ivec4 normals;
        glm::ivec4 texcoords;
    };
    std::vector<Face> m_faces;



};
