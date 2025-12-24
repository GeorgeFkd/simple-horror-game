#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include "GlMacros.h"


void deleteBuffer(GLuint *buffer);
void deleteVertexArray(GLuint *vao);

void makeVertexArray(GLuint *vao);
void makeBuffer(GLuint *buffer);
void bindVAO(GLuint vao);
void unbindVAO();
void bindBufferData(unsigned int size, void* ptr);
void bindBuffer(GLuint vbo);
void bindElementBuffer(GLuint ebo);
void bindElementBufferData(unsigned int size,void* ptr);

void enableVAttribArray(unsigned int);
void bindVAttribPointer(unsigned int,unsigned int,std::size_t size,void* offsetPtr);

void enable_gl_features(std::initializer_list<GLenum> features);
void set_viewport(int x, int y, int width, int height);
void bind_framebuffer(GLenum target, GLuint framebuffer);
void gl_clear();
void clear_buffers(GLbitfield mask);
void disable_gl_capability(GLenum cap);

void set_cull_face(GLenum face);

void set_color_mask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void unbind_shader();

GLuint create_framebuffer();
GLuint create_depth_cubemap(unsigned width, unsigned height);
GLuint create_depth_texture_2d(unsigned width, unsigned height);
void attach_depth_texture(GLuint fbo, GLuint texture, bool cubemap);
void validate_framebuffer();
void disable_color_buffers();
void unbind_framebuffer();
void set_texture_parameter(GLenum target, GLenum pname, GLint param);
void set_texture_parameters(GLenum target, std::initializer_list<std::tuple<GLenum, GLint>> params);

void bind_texture(GLenum target, GLuint texture);
void set_texture_image_2d(GLenum target, GLint level, GLint internalFormat,
                          GLsizei width, GLsizei height, GLint border,
                          GLenum format, GLenum type, const void* data);
void set_texture_parameterfv(GLenum target, GLenum pname, const GLfloat* params);

void attach_texture_to_framebuffer(GLenum target, GLenum attachment, GLuint texture, GLint level);

void attach_texture2d_to_framebuffer(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void set_texture_parameterfv(GLenum target, GLenum pname, const GLfloat* params);

void generate_framebuffer(GLuint* fbo);
void generate_texture(GLuint* texture);

GLenum initialize_glew();
void set_clear_color(float r, float g, float b, float a);

void set_pixel_store(GLenum pname, GLint param);
void unbind_texture(GLenum target);
void set_blend_func(GLenum sfactor, GLenum dfactor);
void set_uniform3f(GLint location, float v0, float v1, float v2);
void activate_texture_unit(GLenum texture);
void draw_arrays(GLenum mode, GLint first, GLsizei count);
void update_buffer_subdata(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
void generate_mipmap(GLenum target);

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

    inline void unbind() {
        glUseProgram(0);
    }


    void bindVAO(GLuint vao);
    void unbindVAO();
    void drawElemTriangles(unsigned int count,void* offset_ptr);

    void set_bool(const std::string &name, bool value);         
    void set_int (const std::string &name, int value); 
    void set_float(const std::string &name, float value); 
    void set_vec2(const std::string &name, const glm::vec2 &v);
    void set_vec2(const std::string &name, float x, float y);
    void set_vec3(const std::string &name, const glm::vec3 &v);
    void set_vec3(const std::string &name, float x, float y, float z);
    void set_vec4(const std::string &name, const glm::vec4 &v);
    void set_vec4(const std::string &name, float x, float y, float z, float w);
    void set_mat2(const std::string &name, const glm::mat2 &m);
    void set_mat3(const std::string &name, const glm::mat3 &m);
    void set_mat4(const std::string &name, const glm::mat4 &m);

    void set_texture(const std::string& name, GLuint texture, GLenum unit = GL_TEXTURE0, GLenum target = GL_TEXTURE_2D);


    Shader(const std::vector<std::string>& shader_paths,
           const std::vector<GLenum>& shader_types,
           const std::string& shader_name);
    ~Shader();
private: 
    GLuint program_id; 
    std::string shader_name;

    std::unordered_map<std::string, GLint> uniform_cache;

};
