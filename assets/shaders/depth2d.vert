
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 lightSpaceMatrix; 

void main() {
    // transform into light’s clip space
    gl_Position = lightSpaceMatrix * uModel * vec4(aPos, 1.0);
}
