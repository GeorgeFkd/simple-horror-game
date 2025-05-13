#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>

class Shader {
public:

    std::string load_file(const std::string& path);
    GLuint compile_shader(GLenum type, const std::string& source);
    GLint get_uniform_location(const std::string& name);

    inline GLuint get_shader_program_id(){
        return program_id;
    }
    
    inline std::string get_shader_name(){
        return shader_name;
    }
    
    inline void use(){
        glUseProgram(program_id);
    }

    Shader(const std::vector<std::string>& shader_paths,
           const std::vector<GLenum>& shader_types,
           const std::string& shader_name);
    ~Shader();
private: 
    GLuint program_id; 
    std::string shader_name;

    std::unordered_map<std::string, GLint> uniform_cache;

};