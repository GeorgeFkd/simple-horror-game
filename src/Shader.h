#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include "GlMacros.h"



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

    void set_bool(const std::string &name, bool value);         
    void set_int (const std::string &name, int value); 
    void set_float(const std::string &name, float value); 
    void set_vec2(const std::string &name, const glm::vec2 &v);
    void set_vec2(const std::string &name, float x, float y);
    void set_vec3(const std::string &name, const glm::vec3 &v);
    void set_vec3(const std::string &name, float x, float y, float z);
    void set_vec4(const std::string &name, const glm::vec4 &v);
    void set_vec4(const std::string &name, float x, float y, float z, float w);
    void set_mat2(const std::string &name, const glm::mat2 &m);
    void set_mat3(const std::string &name, const glm::mat3 &m);
    void set_mat4(const std::string &name, const glm::mat4 &m);

    void set_texture(const std::string& name, GLuint texture, GLenum unit = GL_TEXTURE0, GLenum target = GL_TEXTURE_2D);


    Shader(const std::vector<std::string>& shader_paths,
           const std::vector<GLenum>& shader_types,
           const std::string& shader_name);
    ~Shader();
private: 
    GLuint program_id; 
    std::string shader_name;

    std::unordered_map<std::string, GLint> uniform_cache;

};
