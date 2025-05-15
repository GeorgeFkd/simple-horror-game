#include "Light.h"
#include <iostream>
Light::Light(
    LightType light_type, 
    const glm::vec3& position, 
    const glm::vec3& direction,
    const glm::vec3& ambient, 
    const glm::vec3& diffuse,
    const glm::vec3& specular,
    float cutoff, // inner cone
    float outer_cutoff, // outer cone
    int shadow_width,
    int shadow_height,
    float near_plane,
    float far_plane,
    float ortho_size,
    float attenuation_constant,
    float attenuation_linear,
    float attenuation_quadratic,
    float attenuation_power,
    float light_intensity)
    : type(light_type), 
      position(position), 
      direction(direction), 
      ambient(ambient), 
      diffuse(diffuse),
      specular(specular), 
      cutoff(cutoff), 
      outer_cutoff(outer_cutoff),
      shadow_height(shadow_height),
      shadow_width(shadow_width), 
      near_plane(near_plane), 
      far_plane(far_plane),
      ortho_size(ortho_size), 
      attenuation_constant(attenuation_constant), 
      attenuation_linear(attenuation_linear),
      attenuation_quadratic(attenuation_quadratic),
      attenuation_power(attenuation_power),
      light_intensity(light_intensity)
      {

    glGenFramebuffers(1, &depth_map_fbo);

    if (type == LightType::POINT) {
        glGenTextures(1, &depth_map);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depth_map);
        for (unsigned i = 0; i < 6; ++i) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_DEPTH_COMPONENT,
                shadow_width, shadow_height,
                0, GL_DEPTH_COMPONENT,
                GL_FLOAT, nullptr
            );
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
        glFramebufferTexture(
            GL_FRAMEBUFFER, 
            GL_DEPTH_ATTACHMENT, 
            depth_map, 
            0
        );
    } else {
        // -- spot or directional: 2D depth texture --
        glGenTextures(1, &depth_map);
        glBindTexture(GL_TEXTURE_2D, depth_map);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            shadow_width, shadow_height,
            0, GL_DEPTH_COMPONENT,
            GL_FLOAT, nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D,
            depth_map,
            0
        );
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Light::draw_lighting(Shader *shader, const std::string &base, int index) const{
    shader->set_vec3(base + "position", position);
    // only send direction for non-point lights
    if (type != LightType::POINT){
        shader->set_vec3(base + "direction", direction);
    }
    shader->set_vec3(base + "ambient", ambient);
    shader->set_vec3(base + "diffuse", diffuse);
    shader->set_vec3(base + "specular", specular);
    if (type == LightType::SPOT)
    {
        shader->set_float(base + "cutoff", cutoff);
        shader->set_float(base + "outerCutoff", outer_cutoff);
    }

    shader->set_int(base + "type", int(type));

    glm::mat4  light_space_matrix = get_light_view() * get_light_projection();
    shader->set_mat4(base + "lightSpaceMatrix", light_space_matrix);
    //shader->set_texture(base + "shadowMap", get_depth_texture(), 5);
}

void Light::draw_depth_pass(Shader* shader) const {
    glViewport(0, 0, shadow_width, shadow_height);
    glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0);
    glClear(GL_DEPTH_BUFFER_BIT);

    shader->use();

    if (type == LightType::POINT) {
        glm::mat4 proj = get_light_projection();
        auto views = get_point_light_views();
        for (int i = 0; i < 6; ++i) {
            shader->set_mat4("shadowMatrices[" + std::to_string(i) + "]", proj*views[i]);
        }
        shader->set_float("far_plane", far_plane);
        shader->set_vec3("light_pos", position);
    } else {
        glm::mat4 light_space = get_light_projection() * get_light_view();
        shader->set_mat4("uLightSpaceMatrix", light_space);
    }
}

void Light::bind_shadow_map(Shader* shader, const std::string& base, int index) const{

    // pick the GLSL sampler name and GL bind‐target
    // point lights use a cube‐map
    if (type == LightType::POINT){
        //shader->set_int(base + "shadowMapCube", index);
        shader->set_texture(base + "shadowMapCube",
                            get_depth_texture(),
                            GL_TEXTURE0 + index,
                            GL_TEXTURE_CUBE_MAP);
    }else{
        // spot or directional use a 2D depth map
        //shader->set_int(base + "shadowMap2D", index);
        shader->set_texture(base + "shadowMap2D",
                            get_depth_texture(),
                            GL_TEXTURE0 + index,
                            GL_TEXTURE_2D);
    }
}


glm::mat4 Light::get_light_projection() const
{
    switch (type)
    {
    case LightType::DIRECTIONAL:
    {
        // Ortho dims: half‐width/height of the shadow volume
        float orthoSize = this->ortho_size;
        return glm::ortho(
            -ortho_size, +ortho_size,
            -ortho_size, +ortho_size,
            near_plane, far_plane
        );
    }
    case LightType::POINT:
    {
        // For point lights we render a cubemap: 90° FOV and square aspect
        return glm::perspective(
            glm::radians(90.0f),
            1.0f,
            near_plane,
            far_plane
        );
    }
    case LightType::SPOT:
    {
        // Use the spot cone angle as FOV (double the cutoff half‐angle)
        float fov = glm::radians(outer_cutoff * 2.0f);
        float aspect = (float)shadow_width / (float)shadow_height;
        return glm::perspective(
            fov,
            aspect,
            near_plane,
            far_plane
        );
    }
    }
}

glm::mat4 Light::get_light_view() const {
    // For directional and spot lights
    return glm::lookAt(
        position,
        position + direction,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

std::vector<glm::mat4> Light::get_point_light_views() const {
    return {
        glm::lookAt(position, position + glm::vec3(1, 0, 0),   glm::vec3(0, -1, 0)), // +X
        glm::lookAt(position, position + glm::vec3(-1, 0, 0),  glm::vec3(0, -1, 0)), // -X
        glm::lookAt(position, position + glm::vec3(0, 1, 0),   glm::vec3(0, 0, 1)),  // +Y
        glm::lookAt(position, position + glm::vec3(0, -1, 0),  glm::vec3(0, 0, -1)), // -Y
        glm::lookAt(position, position + glm::vec3(0, 0, 1),   glm::vec3(0, -1, 0)), // +Z
        glm::lookAt(position, position + glm::vec3(0, 0, -1),  glm::vec3(0, -1, 0)), // -Z
    };
}