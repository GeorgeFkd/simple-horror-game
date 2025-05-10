#include "SceneManager.h"
#include <iostream>

SceneManager::SceneManager::SceneManager(int width, int height)
{
  auto vert_src = load_file("assets/shaders/passthrough.vert");
  auto frag_src = load_file("assets/shaders/passthrough.frag");

  GLuint vert = compile_shader(GL_VERTEX_SHADER,   vert_src);
  GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);

  shader_program = glCreateProgram();
  glAttachShader(shader_program, vert);
  glAttachShader(shader_program, frag);
  glLinkProgram(shader_program);

  GLint ok = GL_FALSE;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &ok);
  if (ok != GL_TRUE) {
    GLint len = 0;
    glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetProgramInfoLog(shader_program, len, &len, &log[0]);
    throw std::runtime_error("Shader link failed:\n" + log);
  }

  glDeleteShader(vert);
  glDeleteShader(frag);
}

void SceneManager::SceneManager::add_model(Model::Model& model){
  models.push_back(&model);
}


std::string SceneManager::SceneManager::load_file(const std::string& path){
  std::ifstream file(path);
  if (!file) throw std::runtime_error("Failed to open file: " + path);

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}



GLuint SceneManager::SceneManager::compile_shader(GLenum type, const std::string& source){
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

void SceneManager::SceneManager::render(const glm::mat4& view_projection){
  for (auto const& model: models){
    model->draw(view_projection);
  }
}