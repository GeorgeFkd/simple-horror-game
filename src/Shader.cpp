#include "Shader.h"
#include <iostream>

using namespace GlHelpers;

void makeVertexArray(GLuint* vao) {
    GLCall(glGenVertexArrays(1, vao));
}

void makeBuffer(GLuint* buffer) {
    GLCall(glGenBuffers(1, buffer));
}
void deleteBuffer(GLuint* buffer) {
    if (*buffer) {
        GLCall(glDeleteBuffers(1, buffer));
        *buffer = 0;
    }
}

void deleteVertexArray(GLuint* vao) {
    if (*vao) {
        GLCall(glDeleteVertexArrays(1, vao));
        *vao = 0;
    }
}

void bindVAO(GLuint vao) {
    GLCall(glBindVertexArray(vao));
}
void unbindVAO() {
    GLCall(glBindVertexArray(0));
}
void bindBuffer(GLuint vbo) {

    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
}

void bindBufferData(unsigned int size, void* ptr) {
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, ptr, GL_STATIC_DRAW));
}

void bindElementBuffer(GLuint ebo){
     GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
}

void bindElementBufferData(unsigned int size,void* ptr) {
GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, ptr, GL_STATIC_DRAW));

}
void enableVAttribArray(unsigned int arr){
    GLCall(glEnableVertexAttribArray(arr));
}
void bindVAttribPointer(unsigned int start,unsigned  int end,std::size_t size,void* offsetPtr){
    GLCall(glVertexAttribPointer(start , end, GL_FLOAT, GL_FALSE, size, 
                             offsetPtr));
}

void enable_gl_features(std::initializer_list<GLenum> features)
{
    for (GLenum feature : features) {
        GLCall(glEnable(feature));
    }
}


void set_viewport(int x, int y, int width, int height)
{
    GLCall(glViewport(x, y, width, height));
}

void bind_framebuffer(GLenum target, GLuint framebuffer)
{
    GLCall(glBindFramebuffer(target, framebuffer));
}

GLenum initialize_glew()
{
    GLenum status = glewInit();
    if (status != GLEW_OK) {
        std::cerr << "GLEW Initialization failed: " << glewGetErrorString(status) << "\n";
    }
    return status;
}

void set_pixel_store(GLenum pname, GLint param)
{
    GLCall(glPixelStorei(pname, param));
}

void set_blend_func(GLenum sfactor, GLenum dfactor)
{
    GLCall(glBlendFunc(sfactor, dfactor));
}

void draw_arrays(GLenum mode, GLint first, GLsizei count)
{
    GLCall(glDrawArrays(mode, first, count));
}

void update_buffer_subdata(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
    GLCall(glBufferSubData(target, offset, size, data));
}


void unbind_texture(GLenum target)
{
    GLCall(glBindTexture(target, 0));
}

void clear_buffers(GLbitfield mask)
{
    GLCall(glClear(mask));
}

void disable_gl_capability(GLenum cap)
{
    GLCall(glDisable(cap));
}


void gl_clear()
{
    GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void set_cull_face(GLenum face)
{
    GLCall(glCullFace(face));
}

void set_color_mask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    GLCall(glColorMask(red, green, blue, alpha));
}
void set_clear_color(float r, float g, float b, float a)
{
    GLCall(glClearColor(r, g, b, a));
}


void unbind_shader()
{
    GLCall(glUseProgram(0));
}

GLuint create_framebuffer()
{
    GLuint fbo = 0;
    GLCall(glGenFramebuffers(1, &fbo));
    return fbo;
}

GLuint create_depth_cubemap(unsigned width, unsigned height)
{
    GLuint texture = 0;
    GLCall(glGenTextures(1, &texture));
    GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, texture));

    for (unsigned i = 0; i < 6; ++i)
    {
        GLCall(glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_DEPTH_COMPONENT,
            width,
            height,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        ));
    }

    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

    return texture;
}

GLuint create_depth_texture_2d(unsigned width, unsigned height)
{
    GLuint texture = 0;
    GLCall(glGenTextures(1, &texture));
    GLCall(glBindTexture(GL_TEXTURE_2D, texture));

    GLCall(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        width,
        height,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    ));

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));

    float border_color[] = {1.f, 1.f, 1.f, 1.f};
    GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color));

    return texture;
}

void set_texture_parameter(GLenum target, GLenum pname, GLint param)
{
    GLCall(glTexParameteri(target, pname, param));
}
void set_texture_parameters(GLenum target, std::initializer_list<std::tuple<GLenum, GLint>> params)
{
    for (const auto& p : params)
    {
        GLenum pname;
        GLint param;
        std::tie(pname, param) = p;
        GLCall(glTexParameteri(target, pname, param));
    }
}

void bind_texture(GLenum target, GLuint texture)
{
    GLCall(glBindTexture(target, texture));
}

void set_texture_image_2d(GLenum target, GLint level, GLint internalFormat,
                          GLsizei width, GLsizei height, GLint border,
                          GLenum format, GLenum type, const void* data)
{
    GLCall(glTexImage2D(target, level, internalFormat, width, height, border, format, type, data));
}



void generate_framebuffer(GLuint* fbo)
{
    GLCall(glGenFramebuffers(1, fbo));
}

void generate_texture(GLuint* texture)
{
    GLCall(glGenTextures(1, texture));
}

void attach_depth_texture(GLuint fbo, GLuint texture, bool cubemap)
{
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    if (cubemap)
    {
        GLCall(glFramebufferTexture(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            texture,
            0
        ));
    }
    else
    {
        GLCall(glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            texture,
            0
        ));
    }
}

void attach_texture_to_framebuffer(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
    GLCall(glFramebufferTexture(target, attachment, texture, level));
}

void validate_framebuffer()
{
    GLCall(GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Framebuffer not complete! Status: " << status << "\n";
    }
}

void disable_color_buffers()
{
    GLCall(glDrawBuffer(GL_NONE));
    GLCall(glReadBuffer(GL_NONE));
}

void unbind_framebuffer()
{
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void attach_texture2d_to_framebuffer(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    GLCall(glFramebufferTexture2D(target, attachment, textarget, texture, level));
}

void set_texture_parameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
    GLCall(glTexParameterfv(target, pname, params));
}



// void set_blend_func(GLenum sfactor, GLenum dfactor)
// {
//     GLCall(glBlendFunc(sfactor, dfactor));
// }

void set_uniform3f(GLint location, float v0, float v1, float v2)
{
    GLCall(glUniform3f(location, v0, v1, v2));
}

void activate_texture_unit(GLenum texture)
{
    GLCall(glActiveTexture(texture));
}




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
        GLuint      shader        = compile_shader(shader_types[i], shader_source);

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

GLuint Shader::compile_shader(GLenum type, const std::string& source) {
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

    if (success == GL_FALSE) {
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

std::string Shader::load_file(const std::string& path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLint Shader::get_uniform_location(const std::string& name) {
    // check cache
    auto it = uniform_cache.find(name);
    if (it != uniform_cache.end())
        return it->second;

    GLCall(GLint loc = glGetUniformLocation(program_id, name.c_str()));
    if (loc < 0) {
        std::cerr << "WARNING: Uniform '" << name << "' not found in shader '" << shader_name
                  << "'\n";
    }
    uniform_cache[name] = loc;
    return loc;
}

// Macro to reduce repetition
#define SET_UNIFORM(loc, call)                                                                     \
    if (loc < 0)                                                                                   \
        return;                                                                                    \
    call;                                                                                          \
    {                                                                                              \
    }

void Shader::set_bool(const std::string& name, bool v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform1i(loc, (int)v));
}
void Shader::set_int(const std::string& name, int v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform1i(loc, v));
}
void Shader::set_float(const std::string& name, float v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform1f(loc, v));
}
void Shader::set_vec2(const std::string& name, const glm::vec2& v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform2fv(loc, 1, glm::value_ptr(v)));
}
void Shader::set_vec2(const std::string& name, float x, float y) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform2f(loc, x, y));
}
void Shader::set_vec3(const std::string& name, const glm::vec3& v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform3fv(loc, 1, glm::value_ptr(v)));
}
void Shader::set_vec3(const std::string& name, float x, float y, float z) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform3f(loc, x, y, z));
}
void Shader::set_vec4(const std::string& name, const glm::vec4& v) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform4fv(loc, 1, glm::value_ptr(v)));
}
void Shader::set_vec4(const std::string& name, float x, float y, float z, float w) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniform4f(loc, x, y, z, w));
}
void Shader::set_mat2(const std::string& name, const glm::mat2& m) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniformMatrix2fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
}
void Shader::set_mat3(const std::string& name, const glm::mat3& m) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
}
void Shader::set_mat4(const std::string& name, const glm::mat4& m) {
    GLint loc = get_uniform_location(name);
    SET_UNIFORM(loc, glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m)));
}
void Shader::set_texture(const std::string& name, GLuint tex, GLenum unit, GLenum target) {
    GLint loc = get_uniform_location(name);
    if (loc < 0) {
        std::cerr << "ERROR: Cannot bind texture to missing uniform '" << name << "' in shader '"
                  << shader_name << "'\n";
        return;
    }
    GLCall(glActiveTexture(unit));
    GLCall(glBindTexture(target, tex));
    GLCall(glUniform1i(loc, unit - GL_TEXTURE0));
}

#undef SET_UNIFORM

Shader::~Shader() {
    if (program_id != 0) {
        glDeleteProgram(program_id);
        program_id = 0;
    }
}

void Shader::bindVAO(GLuint vao) {
    GLCall(glBindVertexArray(vao));
}

void Shader::drawElemTriangles(unsigned int count, void* offset_ptr) {
    GLCall(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, offset_ptr));
}

void Shader::unbindVAO() {
    GLCall(glBindVertexArray(0));
}
