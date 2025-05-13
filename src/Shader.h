#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

class Shader {
public:
    std::string load_file(const std::string& path);
    GLuint compile_shader(GLenum type, const std::string& source);

    inline GLuint get_shader_program_id(){
        return program_id;
    }
    
    inline std::string get_shader_name(){
        return shader_name;
    }

    Shader(const std::vector<std::string>& shader_paths,
           const std::vector<GLenum>& shader_types,
           const std::string& shader_name);
    ~Shader();
private: 
    GLuint program_id; 
    std::string shader_name;
};