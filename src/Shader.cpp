#include "Shader.h"
#include <iostream>


Shader::Shader(const std::vector<std::string>& shader_paths,
           const std::vector<GLenum>& shader_types,
           const std::string& shader_name) 
    {
        if (shader_paths.size() != shader_types.size()) {
            throw std::runtime_error("Shader constructor: Mismatched input vector sizes.");
        }

        GLCall(program_id = glCreateProgram());
        std::vector<GLuint> compiled_shaders;

        for (size_t i = 0; i < shader_paths.size(); ++i) {
            std::string shader_source = load_file(shader_paths[i]);
            GLuint shader = compile_shader(shader_types[i], shader_source);

            GLCall(glAttachShader(program_id, shader));
            compiled_shaders.push_back(shader);
        }

        GLCall(glLinkProgram(program_id));

        // Check for linking errors
        GLint success;
        glGetProgramiv(program_id, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program_id, 512, nullptr, infoLog);
            throw std::runtime_error(std::string("Shader linking failed: ") + infoLog);
        }

        // Clean up shaders (they are already linked into the program)
        for (GLuint shader : compiled_shaders) {
            GLCall(glDetachShader(program_id, shader));
            GLCall(glDeleteShader(shader));
        }

        this->shader_name = shader_name;
    }

GLuint Shader::compile_shader(GLenum type, const std::string& source){
    // Every symbolic constant you pass to an OpenGL 
    // function—like GL_ARRAY_BUFFER, GL_TRIANGLES, GL_FLOAT, 
    // GL_BLEND, etc.—is actually just an integer constant 
    // of type GLenum.
    // glBindBuffer((GLenum)0x8892, vbo);
    GLCall(GLuint shader = glCreateShader(type));
    const char* src = source.c_str();
    GLCall(glShaderSource(shader, 1, &src, nullptr));
    GLCall(glCompileShader(shader));

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if(success == GL_FALSE){
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(max_length);
        glGetShaderInfoLog(shader, max_length, &max_length, &errorLog[0]);
        
        // convert to std::string
        std::string error_message(errorLog.begin(), errorLog.end());

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.

        std::cerr << error_message << std::endl;

        // throw with the full message
        throw std::runtime_error("Shader compilation failed:\n" + error_message);
    }

    return shader;
}

std::string Shader::load_file(const std::string& path){
  std::ifstream file(path);
  if (!file) throw std::runtime_error("Failed to open file: " + path);

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

GLint Shader::get_uniform_location(const std::string& name) {
    // check cache
    auto it = uniform_cache.find(name);
    if (it != uniform_cache.end()) return it->second;

    GLCall(GLint loc = glGetUniformLocation(program_id, name.c_str()));
    if (loc < 0) {
        std::cerr << "WARNING: Uniform '" << name
                  << "' not found in shader '" << shader_name << "'\n";
    }
    uniform_cache[name] = loc;
    return loc;
}

// Macro to reduce repetition
#define SET_UNIFORM(loc, call)                 \
    if (loc < 0) return;                       \
    call;                                      \
    {}     

void Shader::set_bool(const std::string &name, bool v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform1i(loc, (int)v));
}
void Shader::set_int(const std::string &name, int v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform1i(loc, v));
}
void Shader::set_float(const std::string &name, float v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform1f(loc, v));
}
void Shader::set_vec2(const std::string &name, const glm::vec2 &v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform2fv(loc, 1, glm::value_ptr(v)));
}
void Shader::set_vec2(const std::string &name, float x, float y) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform2f(loc, x, y));
}
void Shader::set_vec3(const std::string &name, const glm::vec3 &v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform3fv(loc, 1, glm::value_ptr(v)));
}
void Shader::set_vec3(const std::string &name, float x, float y, float z) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform3f(loc, x, y, z));
}
void Shader::set_vec4(const std::string &name, const glm::vec4 &v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform4fv(loc, 1, glm::value_ptr(v)));
}
void Shader::set_vec4(const std::string &name, float x, float y, float z, float w) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform4f(loc, x, y, z, w));
}
void Shader::set_mat2(const std::string &name, const glm::mat2 &m) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniformMatrix2fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
}
void Shader::set_mat3(const std::string &name, const glm::mat3 &m) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
}
void Shader::set_mat4(const std::string &name, const glm::mat4 &m) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
}
void Shader::set_texture(
    const std::string& name, 
    GLuint tex, 
    GLenum unit,
    GLenum target
) {
    GLint loc = get_uniform_location(name);
    if (loc < 0) {
        std::cerr << "ERROR: Cannot bind texture to missing uniform '"
                  << name << "' in shader '" << shader_name << "'\n";
        return;
    }
    GLCall(glActiveTexture(unit));
    GLCall(glBindTexture(target, tex));
    GLCall(glUniform1i(loc, unit - GL_TEXTURE0));
}

#undef SET_UNIFORM

Shader::~Shader(){
    if (program_id!= 0) {
        glDeleteProgram(program_id);
        program_id = 0;
    }
}
