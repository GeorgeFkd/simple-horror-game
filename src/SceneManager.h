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
#include "Light.h"

namespace SceneManager{


    class SceneManager{
    public: 

        // TODO very minimal function to load shaders
        std::string load_file(const std::string& path);
        // TODO very minimal function to compile shaders
        GLuint compile_shader(GLenum type, const std::string& source);
        void add_model(Model::Model& model);
        void render(const glm::mat4& view_projection);

        GLuint get_shader_program();

        inline const std::vector<Model::Model*> get_models() const{
            return models;
        }

        void add_light(const Light& L) { lights.push_back(L); }

        void set_spotlight(size_t idx, const glm::vec3& pos, const glm::vec3& dir) {
            if (idx < lights.size()) {
            lights[idx].position  = pos;
            lights[idx].direction = dir;
            }
        }

        SceneManager(int width, int height);
        ~SceneManager();
    private:

        std::vector<Model::Model*> models;
        std::vector<Light> lights;

        GLuint shader_program;
        GLsizei index_count = 0;
        int screen_width, screen_height;

    };

}