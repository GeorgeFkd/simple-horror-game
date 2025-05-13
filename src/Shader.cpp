#include "Shader.h"


Shader::Shader(const std::vector<std::string>& shader_paths,
           const std::vector<GLenum>& shader_types,
           const std::string& shader_name) 
    {
        if (shader_paths.size() != shader_types.size()) {
            throw std::runtime_error("Shader constructor: Mismatched input vector sizes.");
        }

        program_id = glCreateProgram();
        std::vector<GLuint> compiled_shaders;

        for (size_t i = 0; i < shader_paths.size(); ++i) {
            std::string shader_source = load_file(shader_paths[i]);
            GLuint shader = compile_shader(shader_types[i], shader_source);

            glAttachShader(program_id, shader);
            compiled_shaders.push_back(shader);
        }

        glLinkProgram(program_id);

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
            glDetachShader(program_id, shader);
            glDeleteShader(shader);
        }

        this->shader_name = shader_name;
    }

GLuint Shader::compile_shader(GLenum type, const std::string& source){
    // Every symbolic constant you pass to an OpenGL 
    // function—like GL_ARRAY_BUFFER, GL_TRIANGLES, GL_FLOAT, 
    // GL_BLEND, etc.—is actually just an integer constant 
    // of type GLenum.
    // glBindBuffer((GLenum)0x8892, vbo);
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if(success == GL_FALSE){
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(max_length);
        glGetShaderInfoLog(shader, max_length, &max_length, &errorLog[0]);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        return -1;
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
    auto it = uniform_cache.find(name);
    if (it != uniform_cache.end()) {
        return it->second;
    }

    GLint loc = glGetUniformLocation(program_id, name.c_str());
    uniform_cache[name] = loc;
    return loc;
}


Shader::~Shader(){
    if (program_id!= 0) {
        glDeleteProgram(program_id);
        program_id = 0;
    }
}