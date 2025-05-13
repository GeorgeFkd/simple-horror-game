#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

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
        float outer_cutoff // outer cone
    )
    : type(light_type), 
      position(position), 
      direction(direction), 
      ambient(ambient), 
      diffuse(diffuse), 
      specular(specular), 
      cutoff(cutoff), 
      outer_cutoff(outer_cutoff)
    {}


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

    inline void set_position(glm::vec3& position){
        this->position = position;
    }

    inline void set_direction(glm::vec3& direction){
        this->direction = direction;
    }

    void draw_lighting(GLuint shader_program_id, const std::string& base);
private:
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float cutoff;  // inner cone
    float outer_cutoff;  // outer cone
};