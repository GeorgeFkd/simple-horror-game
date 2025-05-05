#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <charconv> 
#include <cstring>
#include <stdexcept>
#include <cctype>
#include <string>
//should probably use data structures from the libraries

#define DEBUG_OBJLOADER


namespace ObjectLoader{ 

    struct Vertex {
        glm::vec3 position; 
        glm::vec2 texcoord; 
        glm::vec3 normal;
    };

    struct Face {
        glm::ivec4 vertices;
        glm::ivec4 normals;
        glm::ivec4 texcoords;
    };


    enum class LineType{Vertex, Texcoord, Normal, Face, Unknown};

    inline LineType classify_line_type(std::string_view kw) {
    if      (kw == "v")  return LineType::Vertex;
    else if (kw == "vt") return LineType::Texcoord;
    else if (kw == "vn") return LineType::Normal;
    else if (kw == "f")  return LineType::Face;
    else                 return LineType::Unknown;
    }


    // Parse exactly N floats from buff into out[0..N-1].
    // Missing trailing floats will be zeroed.
    template<int N>
    bool parse_components(const char* buff, float out[N]) {
        const char* end = buff + std::strlen(buff);
        const char* p   = buff;
        for (int i = 0; i < N; ++i) {
            // parse next float
            auto [next, ec] = std::from_chars(p, end, out[i]);
            if (ec != std::errc()) {
                // if the very first float failed, give up entirely
                if (i == 0) return false;
                // otherwise assume the rest are missing → zero them
                for (++i; i < N; ++i) out[i] = 0.f;
                return true;
            }
            // skip whitespace before the next read
            p = next;
            while (p < end && std::isspace(*p)) ++p;
        }
        return true;
    }

    class OBJLoader{
    private: 

        void read_normal(const char* buff);
        void read_vertex(const char* buff);
        void read_texcoord(const char* buff);
        void read_faceLimited(const char* buff);
        void read_usemtl(const char* buff, int &material_id);
        void read_mtllib(const char* buff, const std::string& filename);
        void add_new_group(const char* buff, int &material_id);

    public:
        void read_from_file(const std::string& filename);

        std::vector<glm::vec4> m_vertices;
        std::vector<glm::vec3> m_texture_coords;  
        std::vector<glm::vec3> m_vertex_normals;


        std::vector<Face> m_faces;

        void parseFace(const std::string &line);

        inline void print_glmvec3(glm::vec3 v) {
            std::cout << "Read GLM Vec 3 from .obj: " << v.x << "," << v.y << "," << v.z << "\n";
        }

        inline void print_glmvec4(glm::vec4 v) {
            std::cout << "Read GLM Vec 4 from .obj: " << v.x << "," << v.y << "," << v.z << "," << v.w << "\n";
        }

        inline void print_glmvec2(glm::vec2 v) { 
            std::cout << "Read GLM Vec 2 from .obj: " << v.x << "," << v.y << "\n";
        }

        OBJLoader() = default;
        ~OBJLoader() = default;
    };

}