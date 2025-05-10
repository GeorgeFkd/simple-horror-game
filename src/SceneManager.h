#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <string>
#include <algorithm>
#include "OBJLoader.h"
#include "Model.h"

namespace SceneManager{


    class SceneManager{
    public: 

        // TODO very minimal function to load shaders
        std::string load_file(const std::string& path);
        // TODO very minimal function to compile shaders
        GLuint compile_shader(GLenum type, const std::string& source);
        void add_model(Model::Model& model);
        void render(const glm::mat4& view_projection);

        SceneManager(int width, int height);
        ~SceneManager(){};
    private:

        std::vector<Model::Model*> models;
        GLuint shader_program;
        GLsizei index_count = 0;
        int screen_width, screen_height;

    };

}