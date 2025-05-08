#pragma once
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
        inline glm::mat4 getViewMatrix() const {
            return glm::lookAt(position, position + front, up);
        }

        inline glm::mat4 getProjectionMatrix() const {
            return glm::perspective(fov, aspect, near_z, far_z);
        }

        void updateCameraVectors() {
            // calculate new front vector
            glm::vec3 f;
            // contribution on x axis * how much we are rotared on the y
            f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            // contribution on y axis
            f.y = sin(glm::radians(pitch));
            // contribution on z axis * how much we are rotared on the y
            f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(f);

            // re‐compute right and up
            right = glm::normalize(glm::cross(front, world_up));
            up    = glm::normalize(glm::cross(right, front));
        }
    };
}