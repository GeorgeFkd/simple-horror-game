#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Renderer{
public: 

    Renderer(int width, int height);
    ~Renderer();
private:
    GLuint vao, vbo;
    GLuint shaderProgram;
    int screenWidth, screenHeight;

    GLuint LoadShader(const std::string& vertPath, const std::string& fragPath);
    std::string LoadFile(const std::string& path);
};