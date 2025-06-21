#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <string_view>
#include "Shader.h"
#include "Model.h"
#include "fwd.hpp"
#include "GlMacros.h"
#include "Camera.h"

enum class LightType { 
    POINT = 0,
    DIRECTIONAL = 1, 
    SPOT = 2
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
        float light_power = 1.0f,
        bool is_on = true,
        std::string_view label = "",
        glm::vec3 color = glm::vec3(1.0f)
    );
    
    inline void set_name(std::string_view name) {
        label = name;
    }

    inline std::string_view name() {
        return label;
    }
    
    inline void set_turned_on(bool on) {
        is_on = on;
    }
    inline bool is_turned_on() const {
        return is_on;
    };
    
    inline void set_light(glm::vec3 new_color) {
        color = new_color;
    }

    inline void make_light_red() {
        color = glm::vec3(1.0f,0.0f,0.0f);
    }

    inline void make_light_green() {
        color = glm::vec3(0.0f,1.0f,0.0f);
    }

    inline void make_light_blue() {
        color = glm::vec3(0.0f,0.0f,1.0f);
    }

    inline void  toggle_light() {
        if(is_on) {
            //now being turned off
            prev_light_power = light_power;
            light_power = 0.0f;
            is_on = false;
        }else {
            //being turned back on again
            light_power = prev_light_power;
            is_on = true;
        }
    }

    inline float get_light_power() const {
        return light_power;
    }

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

    void bind_shadow_map(std::shared_ptr<Shader> shader, const std::string& base, int index) const;
    void draw_lighting(std::shared_ptr<Shader> shader, const std::string& base, int index) const;
    void draw_depth_pass(std::shared_ptr<Shader>shader, const std::vector<std::unique_ptr<Models::Model>>& models) const;
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
    float light_power ;
    float prev_light_power;    
    GLuint   depth_map_fbo;
    GLuint   depth_map;

    bool is_on;
    std::string_view label;
    glm::vec3 color;
};
