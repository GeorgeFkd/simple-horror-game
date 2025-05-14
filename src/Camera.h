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
        float aspect = 16.0f / 9.0f;
        float near_z  = 0.1f;
        float far_z   = 100.0f;
        // euler angles
        // yaw represents the magnitude of looking left to right
        // pitch represents how much we are looking up or down 
        float yaw, pitch;

        float collision_radius = 0.3f;    // how “fat” the camera is

        Light* flashlight; 
    public: 
        
        CameraObj(int window_width, int window_height): 
        position(0.0f, 0.0f, 3.0f),
        world_up(0.0f, 1.0f, 0.0f),
        camera_speed(5.0f),
        mouse_sensitivity(0.1f),
        yaw(-90.0f),//import to initialize at -90 to start at 0,0,-1
        pitch(0.0f)
        {
            aspect = float(window_height)/float(window_height);
            updateCameraVectors();
            // cursor is centered
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }

        void update(float delta_time);

        void process_input(const SDL_Event& event);
        //inline glm::vec3 getCameraDirection() const{
        //    return glm::normalize(position - target);
        //}
        //inline glm::vec3 getCameraRight() const{
        //    return glm::normalize(glm::cross(up, getCameraDirection()));
        //}
        //inline glm::vec3 getCameraUp() const{
        //    return glm::cross(getCameraDirection(), getCameraRight());
        //}
        // The below captures all of the above functionality
        inline glm::mat4 get_view_matrix() const {
            return glm::lookAt(position, position + front, up);
        }

        inline glm::mat4 get_projection_matrix() const {
            return glm::perspective(fov, aspect, near_z, far_z);
        }

        inline glm::vec3 get_direction() const {
            return front;
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

        bool intersectSphereAABB(const glm::vec3& cen, float r, const glm::vec3& bmin, const glm::vec3& bmax)
        {
            // find closest point on box to sphere center
            glm::vec3 closest = glm::clamp(cen, bmin, bmax);
            float   dist2   = glm::length2(closest - cen);
            return dist2 <= r*r;
        }

        void updateCameraVectors();
    };
}