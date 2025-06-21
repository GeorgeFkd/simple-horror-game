#pragma once
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Light.h"

namespace Camera{

    class CameraObj{
    private: 
        glm::vec3 position;
        glm::vec3 world_up;
        glm::vec3 front, up, right;

        float camera_speed;
        float mouse_sensitivity;

        float fov    = glm::radians(45.0f);
        float aspect = 0.0f;
        float near_z  = 1.0f;
        float far_z   = 1000.0f;
        // euler angles
        // yaw represents the magnitude of looking left to right
        // pitch represents how much we are looking up or down 
        float yaw, pitch;

        float collision_radius = 0.6f;    // how “fat” the camera is

    public: 
        
        CameraObj(int window_width, int window_height,glm::vec3 position): 
        position(position),
        world_up(0.0f, 3.0f, 0.0f),
        camera_speed(10.0f),
        mouse_sensitivity(0.1f),
        yaw(-90.0f),//import to initialize at -90 to start at 0,0,-1
        pitch(0.0f)
        {
            aspect = float(window_width)/float(window_height);
            updateCameraVectors();
            // cursor is centered
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }

        void update(float delta_time);
        void process_input(const SDL_Event& event);
        const std::array<glm::vec4,6> extract_frustum_planes() const;

        inline glm::mat4 get_view_matrix() const {
            return glm::lookAt(position, position + front, up);
        }

        inline glm::mat4 get_projection_matrix() const {
            return glm::perspective(fov, aspect, near_z, far_z);
        }

        inline glm::vec3 get_direction() const {
            return front;
        }

        inline glm::vec3 get_right() const {
            return right;
        }

        inline glm::vec3 get_position() const {
            return position;
        }

        inline void set_position(const glm::vec3& position){
            this->position = position;
        }

        inline void set_direction(const glm::vec3& direction){
            this->front = direction;
        }

        inline void set_pitch(float pitch){
            this->pitch = pitch;
        }

        inline float get_radius() const {
            return collision_radius;
        }

        void updateCameraVectors();
    };
}
