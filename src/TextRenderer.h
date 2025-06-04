// #include <ft2build.h>
// #include <memory>
// #include "Light.h"
// #include "Shader.h"
// #include FT_FREETYPE_H
// #include <iostream>
// #include <glm.hpp>
// #include <map>
// #include "GlMacros.h"
//
//
//
//
// namespace Text {
//     using namespace GlHelpers;
// unsigned int vao,vbo;
// struct Character {
//     unsigned int textureId;
//     glm::ivec2 size;
//     glm::ivec2 bearing;
//     unsigned int advance;
// };
// std::map<char,Character> characters;
//
// inline void load_font() {
//     FT_Library ft;
//     if (FT_Init_FreeType(&ft)) {
//         std::cout << "ERROR::FREETYPE: Could not initialise Freetype Library\n";
//         return;
//     }
//     std::cout << "Successfully initialised freetype library\n";
//     FT_Face face;
//     auto    fontpath = "assets/fonts/scary.ttf";
//     if (FT_New_Face(ft, fontpath, 0, &face)) {
//         std::cout << "Error in freetype while loading font: " << fontpath << "\n";
//         return;
//     }
//     std::cout << "Successfully loaded font: " << fontpath << "\n";
//
//     FT_Set_Pixel_Sizes(face,0,48);
//     glPixelStorei(GL_UNPACK_ALIGNMENT,1);
//     std::cout << "Successfully set pixel sizes\n";
//
//     if(FT_Load_Char(face,'X',FT_LOAD_RENDER)){
//         std::cout << "Error in freetype while trying to load glyph\n";
//         return;
//     }
//
//
//     for(unsigned char c=0; c < 128; c++) {
//         if(FT_Load_Char(face,c,FT_LOAD_RENDER)){
//             std::cout << "Failed to load glyph: " << c << " .\n";
//             continue;
//         }
//
//         unsigned int texture;
//         GLCall(glGenTextures(1,&texture));
//         GLCall(glBindTexture(GL_TEXTURE_2D,texture));
//         GLCall(glTexImage2D(GL_TEXTURE_2D,0,GL_RED,face->glyph->bitmap.width,face->glyph->bitmap.rows,0,GL_RED,GL_UNSIGNED_BYTE,face->glyph->bitmap.buffer));
//        GLCall(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE));
//         GLCall(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE));
//         GLCall(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
//         GLCall(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR));
//
//         Character character = {
//             texture,
//             glm::ivec2(face->glyph->bitmap.width,face->glyph->bitmap.rows),
//             glm::ivec2(face->glyph->bitmap_left,face->glyph->bitmap_top),
//             //it is a signed long
//             static_cast<unsigned int>(face->glyph->advance.x)
//         };
//         characters.insert(std::pair<char,Character>(c,character));
//     }
//     std::cout << "Characters: " << characters.size() << "\n";
//     glBindTexture(GL_TEXTURE_2D,0);
//
//     FT_Done_Face(face);
//     FT_Done_FreeType(ft);
//
//
//     glGenVertexArrays(1,&vao);
//     glGenBuffers(1,&vbo);
//     glBindVertexArray(vao);
//     glBindBuffer(GL_ARRAY_BUFFER,vbo);
//     glBufferData(GL_ARRAY_BUFFER,sizeof(float) * 6 * 4,NULL,GL_DYNAMIC_DRAW);
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,4 * sizeof(float),0);
//     glBindBuffer(GL_ARRAY_BUFFER,0);
//     glBindVertexArray(0);
//
// }
//
//
//
// inline void RenderText(std::shared_ptr<Shader> s, std::string text,float x,float y, float scale,glm::vec3 color){
//
//     std::cout << "Attempting to render text\n";
//     //not sure where to put this, probably in an init method
//
//
//     s->use();
//     GLCall(glDisable(GL_DEPTH_TEST));
//     GLCall(glEnable(GL_BLEND));
//     GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
//     GLCall(glUniform3f(s->get_uniform_location("textColor"),color.x,color.y,color.z));
//     GLCall(glActiveTexture(GL_TEXTURE0));
//     GLCall(glBindVertexArray(vao));
//
//     std::string::const_iterator c;
//     for(c= text.begin(); c!=text.end(); c++){
//         Character ch = characters[*c];
//
//         float xpos = x + ch.bearing.x * scale;
//         float ypos = y - (ch.size.y - ch.bearing.y) * scale;
//
//         float w = ch.size.x * scale;
//         float h = ch.size.y * scale;
//         // update VBO for each character
//         float vertices[6][4] = {
//             { xpos,     ypos + h,   0.0f, 0.0f },            
//             { xpos,     ypos,       0.0f, 1.0f },
//             { xpos + w, ypos,       1.0f, 1.0f },
//
//             { xpos,     ypos + h,   0.0f, 0.0f },
//             { xpos + w, ypos,       1.0f, 1.0f },
//             { xpos + w, ypos + h,   1.0f, 0.0f }           
//         };
//         // render glyph texture over quad
//         glBindTexture(GL_TEXTURE_2D, ch.textureId);
//         // update content of VBO memory
//         glBindBuffer(GL_ARRAY_BUFFER, vbo);
//         glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
//         glBindBuffer(GL_ARRAY_BUFFER, 0);
//         // render quad
//         glDrawArrays(GL_TRIANGLES, 0, 6);
//         // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
//         x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
//     }
//
//     glBindVertexArray(0);
//     glBindTexture(GL_TEXTURE_2D,0);
//     glEnable(GL_DEPTH_TEST);
//
//     return ;
// }
//
//
// } // namespace Text
//
//
//
//
// TextRenderer.h
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
    void RenderText(std::shared_ptr<Shader> s,
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
