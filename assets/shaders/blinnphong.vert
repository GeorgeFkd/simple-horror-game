#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vFragPos;    // world‐space position
out vec3 vNormal;     // world‐space normal
out vec2 vTexCoord;

void main() {
    // world position of the vertex
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos   = worldPos.xyz;

    // normals must be transformed by the inverse‐transpose of the model
    vNormal    = mat3(transpose(inverse(uModel))) * aNormal;

    vTexCoord  = aTexCoord;
    gl_Position = uProj * uView * worldPos;
}
