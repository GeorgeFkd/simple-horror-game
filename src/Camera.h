#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera{
private: 
    glm::vec3 position {0.0f, 0.0f, 3.0f};
    glm::vec3 target {0.0f, 0.0f, 0.0f};
    glm::vec3 up {0.0f, 1.0f, 0.0f};

    float fov    = glm::radians(45.0f);
    float aspect = 16.0f / 9.0f;
    float near_z  = 0.1f;
    float far_z   = 100.0f;
public: 
    void Update();

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, target, up);
    }

    glm::mat4 getProjectionMatrix() const {
        return glm::perspective(fov, aspect, near_z, far_z);
    }
};