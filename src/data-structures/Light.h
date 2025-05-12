#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

enum LightType { POINT=0, DIRECTIONAL=1, SPOT=2 };

struct Light {
    LightType type        = POINT;
    glm::vec3 position    = glm::vec3(0.0f);
    glm::vec3 direction   = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 ambient     = glm::vec3(0.1f);
    glm::vec3 diffuse     = glm::vec3(0.8f);
    glm::vec3 specular    = glm::vec3(1.0f);
    float     cutoff      = glm::cos(glm::radians(12.5f));  // inner cone
    float     outer_cutoff = glm::cos(glm::radians(17.5f));  // outer cone

    //shadow mapping parameters 
    float near_z = 1.0f;
    float far_z = 25.0f;
    unsigned int shadow_width = 1024;
    unsigned int shadow_height = 1024;

    //attributes for shadowmaps 
    //for spotlights and directional
    GLuint shadow_fbo_2d   = 0;
    GLuint shadow_map_2d   = 0;
    //for point lights
    GLuint shadow_fbo_cube = 0;
    GLuint shadow_map_cube = 0;

    Light(){

        if (type == POINT){
            glGenFramebuffers(1, &shadow_fbo_cube);
            glGenTextures    (1, &shadow_map_cube);
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_map_cube);
            for (int face = 0; face < 6; ++face) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0,
                            GL_DEPTH_COMPONENT,
                            shadow_width, shadow_height, 0,
                            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);
        }else{
            glGenFramebuffers(1, &shadow_fbo_2d);
            glGenTextures    (1, &shadow_map_2d);
            glBindTexture(GL_TEXTURE_2D, shadow_map_2d);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                        shadow_width, shadow_height, 0,
                        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            //set texture attributes
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo_2d);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    GL_TEXTURE_2D, shadow_map_2d, 0);
            glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    ~Light(){
        if (shadow_fbo_2d)   glDeleteFramebuffers(1, &shadow_fbo_2d);
        if (shadow_map_2d)   glDeleteTextures    (1, &shadow_map_2d);
        if (shadow_fbo_cube) glDeleteFramebuffers(1, &shadow_map_cube);
        if (shadow_map_cube) glDeleteTextures    (1, &shadow_map_cube);
    }

    // Directional and spot lights use a single light-space matrix
    glm::mat4 get_light_view_matrix() const {

        // Fallback for non-directional/spot
        if (type == POINT) {
            return glm::mat4(1.0f);
        }

        glm::vec3 up = glm::abs(glm::dot(direction, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f
            ? glm::vec3(0.0f, 0.0f, 1.0f)  // fallback up vector
            : glm::vec3(0.0f, 1.0f, 0.0f);

        return glm::lookAt(position, position + direction, up);

    }

    glm::mat4 get_light_space_matrix() const {
        if (type == POINT){
            return glm::mat4(1.0f);
        }
        return get_light_projection_matrix() * get_light_view_matrix();
    }

    glm::mat4 get_light_projection_matrix() const {
        if (type == DIRECTIONAL) {
            float orthosize = 10.0f;
            return glm::ortho(-orthosize, orthosize, -orthosize, orthosize, near_z, far_z);
        }
        if (type == SPOT) {
            // FOV covers the spot cone
            //cutoff is stored as cos(angle), so we reverse it with acos and convert to degrees.
            //Multiply by 2 to get the full cone angle (not just half).
            float fov = glm::degrees(glm::acos(cutoff)) * 2.0f;
            return glm::perspective(glm::radians(fov), 1.0f, near_z, far_z);
        }
        // POINT light uses cubemap projections per-face
        return glm::mat4(1.0f);
    }

    // Point lights require 6 views for omnidirectional shadow mapping
    std::vector<glm::mat4>  get_point_light_space_matrices() const {
        std::vector<glm::mat4> matrices;
        if (type != POINT) return matrices;

        float aspect = (float) shadow_width / (float) shadow_height;
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), aspect, near_z, far_z);
        
        matrices.reserve(6);
        matrices.push_back(proj * glm::lookAt(position, position + glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // +X
        matrices.push_back(proj * glm::lookAt(position, position + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // -X
        matrices.push_back(proj * glm::lookAt(position, position + glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f))); // +Y
        matrices.push_back(proj * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f))); // -Y
        matrices.push_back(proj * glm::lookAt(position, position + glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // +Z
        matrices.push_back(proj * glm::lookAt(position, position + glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // -Z

        return matrices;
    }
};