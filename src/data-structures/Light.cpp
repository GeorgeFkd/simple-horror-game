#include "Light.h"

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
    float ortho_size)
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
      ortho_size(ortho_size){

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

void Light::draw_lighting(Shader *shader, const std::string &base) const{
    // use the cached location API instead of raw glGetUniformLocation
    glUniform3fv(shader->get_uniform_location(base + "position"), 1, glm::value_ptr(position));
    // only send direction for non-point lights
    if (type != LightType::POINT)
        glUniform3fv(shader->get_uniform_location(base + "direction"), 1, glm::value_ptr(direction));
    glUniform3fv(shader->get_uniform_location(base + "ambient"), 1, glm::value_ptr(ambient));
    glUniform3fv(shader->get_uniform_location(base + "diffuse"), 1, glm::value_ptr(diffuse));
    glUniform3fv(shader->get_uniform_location(base + "specular"), 1, glm::value_ptr(specular));
    if (type == LightType::SPOT)
    {
        glUniform1f(shader->get_uniform_location(base + "cutoff"), cutoff);
        glUniform1f(shader->get_uniform_location(base + "outerCutoff"), outer_cutoff);
    }

    glUniform1i(shader->get_uniform_location(base + "type"), (int)type);
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
            shader->get_uniform_location("shadowMatrices[" + std::to_string(i) + "]");
            glUniformMatrix4fv(
                shader->get_uniform_location("shadowMatrices[" + std::to_string(i) + "]"),
                1, GL_FALSE, glm::value_ptr(proj * views[i])
            );
        }
        glUniform1f(shader->get_uniform_location("far_plane"), far_plane);
        glUniform3fv(shader->get_uniform_location("lightPos"), 1, glm::value_ptr(position));
    } else {
        glm::mat4 light_space = get_light_projection() * get_light_view();
        glUniformMatrix4fv(shader->get_uniform_location("uLightSpaceMatrix"),
                           1, GL_FALSE, glm::value_ptr(light_space));
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