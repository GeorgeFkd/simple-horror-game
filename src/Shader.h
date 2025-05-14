#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    // utility uniform functions
    void set_bool(const std::string &name, bool value){         
        glUniform1i(get_uniform_location(name), (int)value); 
    }
    void set_int(const std::string &name, int value){ 
        glUniform1i(get_uniform_location(name), value); 
    }
    void set_float(const std::string &name, float value){ 
        glUniform1f(get_uniform_location(name), value); 
    }
    void set_vec2(const std::string &name, const glm::vec2 &value){ 
        glUniform2fv(get_uniform_location(name), 1, &value[0]); 
    }
    void set_vec2(const std::string &name, float x, float y){ 
        glUniform2f(get_uniform_location(name), x, y); 
    }
    void set_vec3(const std::string &name, const glm::vec3 &value){ 
        glUniform3fv(get_uniform_location(name), 1, &value[0]); 
    }
    void set_vec3(const std::string &name, float x, float y, float z){ 
        glUniform3f(get_uniform_location(name), x, y, z); 
    }
    void set_vec4(const std::string &name, const glm::vec4 &value){ 
        glUniform4fv(get_uniform_location(name), 1, &value[0]); 
    }
    void set_vec4(const std::string &name, float x, float y, float z, float w) { 
        glUniform4f(get_uniform_location(name), x, y, z, w); 
    }
    void set_mat2(const std::string &name, const glm::mat2 &mat){
        glUniformMatrix2fv(get_uniform_location(name), 1, GL_FALSE, &mat[0][0]);
    }
    void set_mat3(const std::string &name, const glm::mat3 &mat){
        glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, &mat[0][0]);
    }
    void set_mat4(const std::string &name, const glm::mat4 &mat){
        glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &mat[0][0]);
    }
    void set_texture(const std::string& name, GLuint texture, GLenum unit = GL_TEXTURE0){
        GLint location = get_uniform_location(name);
        if (location >= 0) {
            glActiveTexture(unit);
            glBindTexture(GL_TEXTURE_2D, texture);
            glUniform1i(location, unit - GL_TEXTURE0); // Pass texture unit index
        }
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