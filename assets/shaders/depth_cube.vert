#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 shadowMatrices[6];   // the six view-proj matrices

void main() {
    // draw once per face with gl_InstanceID, or if you bind each face
    // set up shadowMatrices[f] before each draw, this will pick
    // the correct one via gl_InstanceID==0,1,2…
    gl_Position = shadowMatrices[gl_InstanceID] * uModel * vec4(aPos, 1.0);
}
