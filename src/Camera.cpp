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

void Camera::CameraObj::process_input(const SDL_Event& event){
    switch(event.type){
        case SDL_MOUSEMOTION:
            yaw   += event.motion.xrel * mouse_sensitivity;
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