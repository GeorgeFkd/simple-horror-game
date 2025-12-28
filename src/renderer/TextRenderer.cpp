// TextRenderer.cpp
#include "TextRenderer.h"
#include "Shader.h"
#include <ft2build.h>
#include FT_FREETYPE_H
void TextRenderer::load_font(const char* fontpath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not initialise FreeType Library\n";
        return;
    }
    std::cout << "Successfully initialised FreeType library\n";

    FT_Face face;
    // const char* fontpath = "assets/fonts/scary.ttf";
    if (FT_New_Face(ft, fontpath, 0, &face)) {
        std::cout << "Error in FreeType while loading font: " << fontpath << "\n";
        FT_Done_FreeType(ft);
        return;
    }
    std::cout << "Successfully loaded font: " << fontpath << "\n";

    FT_Set_Pixel_Sizes(face, 0, 48);
    set_pixel_store(GL_UNPACK_ALIGNMENT, 1);
    std::cout << "Successfully set pixel sizes\n";

    // Preload ASCII 0–127
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "Failed to load glyph: " << c << "\n";
            continue;
        }

        unsigned int texture;
        generate_texture(&texture);           // glGenTextures
        bind_texture(GL_TEXTURE_2D, texture); // glBindTexture

        // Set the texture image
        set_texture_image_2d(GL_TEXTURE_2D,
                             0,                           // level
                             GL_RED,                      // internal format
                             face->glyph->bitmap.width,   // width
                             face->glyph->bitmap.rows,    // height
                             0,                           // border
                             GL_RED,                      // format
                             GL_UNSIGNED_BYTE,            // type
                             face->glyph->bitmap.buffer); // data

        // Set texture parameters using initializer list helper
        set_texture_parameters(GL_TEXTURE_2D, {{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
                                               {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
                                               {GL_TEXTURE_MIN_FILTER, GL_LINEAR},
                                               {GL_TEXTURE_MAG_FILTER, GL_LINEAR}});

        Character character = {texture,
                               glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                               glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                               static_cast<unsigned int>(face->glyph->advance.x)};
        characters.insert(std::pair<char, Character>(c, character));
    }

    std::string other_chars_required = "012345678:/";
    for (auto c : other_chars_required) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "Failed to load glyph: " << c << "\n";
            continue;
        }

        unsigned int texture;
        generate_texture(&texture);           // glGenTextures
        bind_texture(GL_TEXTURE_2D, texture); // glBindTexture

        // New helper needed for glTexImage2D with unsigned formats
        set_texture_image_2d(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                             face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                             face->glyph->bitmap.buffer);

        // Set texture parameters
        set_texture_parameters(GL_TEXTURE_2D, {{GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE},
                                               {GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE},
                                               {GL_TEXTURE_MIN_FILTER, GL_LINEAR},
                                               {GL_TEXTURE_MAG_FILTER, GL_LINEAR}});
        Character character = {texture,
                               glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                               glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                               static_cast<unsigned int>(face->glyph->advance.x)};
        characters.insert(std::pair<char, Character>(c, character));
    }
    std::cout << "Characters loaded: " << characters.size() << "\n";
    unbind_texture(GL_TEXTURE_2D);
    // GLCall(glBindTexture(GL_TEXTURE_2D, 0));

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Setup VAO/VBO for quads
    makeVertexArray(&vao); // glGenVertexArrays
    makeBuffer(&vbo);      // glGenBuffers

    bindVAO(vao);    // glBindVertexArray
    bindBuffer(vbo); // glBindBuffer(GL_ARRAY_BUFFER)

    bindBufferData(sizeof(float) * 6 * 4, nullptr); // glBufferData

    enableVAttribArray(0);                          // glEnableVertexAttribArray
    bindVAttribPointer(0, 4, sizeof(float) * 4, 0); // glVertexAttribPointer

    bindBuffer(0); // unbind GL_ARRAY_BUFFER
    unbindVAO();
}

void TextRenderer::render_text(std::shared_ptr<Shader> s, const std::string& text, float x, float y,
                               float scale, const glm::vec3& color, const glm::mat4& projection) {
    s->use();
    s->set_mat4("projection", projection);

    disable_gl_capability(GL_DEPTH_TEST);
    enable_gl_features({GL_BLEND});
    set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    set_uniform3f(s->get_uniform_location("textColor"), // glUniform3f
                  color.x, color.y, color.z);

    activate_texture_unit(GL_TEXTURE0); // glActiveTexture
    bindVAO(vao);


    for (auto c : text) {
        Character ch = characters[c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // Update VBO for each character
        float vertices[6][4] = {{xpos, ypos + h, 0.0f, 0.0f},    {xpos, ypos, 0.0f, 1.0f},
                                {xpos + w, ypos, 1.0f, 1.0f},

                                {xpos, ypos + h, 0.0f, 0.0f},    {xpos + w, ypos, 1.0f, 1.0f},
                                {xpos + w, ypos + h, 1.0f, 0.0f}};

        bind_texture(GL_TEXTURE_2D, ch.textureId); // glBindTexture

        bindBuffer(vbo); // glBindBuffer(GL_ARRAY_BUFFER)
        update_buffer_subdata(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // glBufferSubData
        bindBuffer(0); // unbind GL_ARRAY_BUFFER

        draw_arrays(GL_TRIANGLES, 0, 6); // glDrawArrays

        // Advance cursors for next glyph (bitshift by 6: 1/64th pixel)
        x += (ch.advance >> 6) * scale;
    }

    unbindVAO();                         // glBindVertexArray(0)
    unbind_texture(GL_TEXTURE_2D);       // glBindTexture(GL_TEXTURE_2D, 0)
    enable_gl_features({GL_DEPTH_TEST}); // glEnable(GL_DEPTH_TEST)
}
