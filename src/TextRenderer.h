#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <map>
#include <string>
#include <memory>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"       // your Shader wrapper

using namespace GlHelpers;

class TextRenderer {
public:
    // Public interface: identical signatures as before
    void load_font();
    void render_text(std::shared_ptr<Shader> s,
                    const std::string& text,
                    float x,
                    float y,
                    float scale,
                    const glm::vec3& color,
                    const glm::mat4& projection);

private:
    struct Character {
        unsigned int   textureId;
        glm::ivec2     size;
        glm::ivec2     bearing;
        unsigned int   advance;
    };

    std::map<char, Character> characters;
    unsigned int              vao = 0;
    unsigned int              vbo = 0;
};

#endif // TEXTRENDERER_H
