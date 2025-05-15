#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Shader.h"

enum class LightType { 
    POINT,
    DIRECTIONAL, 
    SPOT
};

class Light {

public:
    Light(
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
        float attenuation_constant  = 1.0f,
        float attenuation_linear    = 0.35f,
        float attenuation_quadratic = 0.44f,
        float attenuation_power     = 1.0f,
        float light_intensity       = 1.0f
    );


    inline glm::vec3 get_position() const{
        return position;
    };

    inline glm::vec3 get_direction() const{
        return direction;
    }

    inline glm::vec3 get_ambient() const{
        return ambient;
    };

    inline glm::vec3 get_diffuse() const{
        return diffuse;
    }
    
    inline glm::vec3 get_specular() const{
        return specular;
    }

    inline float get_cutoff() const{
        return cutoff;
    }

    inline float get_outer_cutoff() const{
        return outer_cutoff;
    }

    inline LightType get_type() const{
        return type;
    }

    inline GLuint get_depth_texture() const {
        return depth_map;
    }

    inline float get_far_plane() const{
        return far_plane;
    }

    inline float get_near_plane() const{
        return near_plane;
    }

    inline void set_position(const glm::vec3& position){
        this->position = position;
    }

    inline void set_direction(const glm::vec3& direction){
        this->direction = direction;
    }



    glm::mat4 get_light_projection() const;
    glm::mat4 get_light_view() const;
    std::vector<glm::mat4> get_point_light_views() const;

    void draw_lighting(Shader* shader, const std::string& base) const;
    void draw_depth_pass(Shader* shader) const;
private:
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float cutoff;  // inner cone
    float outer_cutoff;  // outer cone

    int shadow_width;
    int shadow_height;
    float near_plane; 
    float far_plane;
    float ortho_size;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
    float attenuation_power;
    float light_intensity;

    GLuint   depth_map_fbo;
    GLuint   depth_map;
};