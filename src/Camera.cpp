#include "Camera.h"


void Camera::CameraObj::update(float delta_time){
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float velocity = camera_speed * delta_time;
    if (keys[SDL_SCANCODE_W]) position += front * velocity;
    if (keys[SDL_SCANCODE_S]) position -= front * velocity;
    if (keys[SDL_SCANCODE_A]) position -= right * velocity;
    if (keys[SDL_SCANCODE_D]) position += right * velocity;
    //TODO temporary just to check
    if (keys[SDL_SCANCODE_Q]) position += world_up * velocity;
    if (keys[SDL_SCANCODE_E]) position -= world_up * velocity;
}

void Camera::CameraObj::updateCameraVectors(){
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

void Camera::CameraObj::process_input(const SDL_Event& event){
    //SDL handles relative mouse motion for you when 
    //using SDL_SetRelativeMouseMode(SDL_TRUE), 
    //while GLFW does not — so in SDL, you don’t need to track 
    //as it says in the OPENGL docs
    //lastX/lastY manually like you do in GLFW
    switch(event.type){
        case SDL_MOUSEMOTION:
            // also causes a LookAt flip once direction vector 
            // is parallel to the world up direction). 
            // The pitch needs to be constrained in such a way 
            // that users won't be able to look higher than 89 degrees 
            // (at 90 degrees we get the LookAt flip) and also not below -89 degrees.
            pitch -= event.motion.yrel * mouse_sensitivity;
            if (pitch >  89.0f) pitch =  89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
            updateCameraVectors();
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                aspect = float(event.window.data1)
                        / float(event.window.data2);
                }
                break;
        default: break;
    }

}