#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uProj;
uniform mat4 uView;

out vec3 FragPos;    // world‐space position
out vec3 Normal;     // world‐space normal
out vec2 TexCoord;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    FragPos   = worldPos.xyz;

    // Transform normal by inverse-transpose of model
    Normal    = mat3(transpose(inverse(uModel))) * aNormal;

    TexCoord  = aTexCoord;
    gl_Position = uProj * uView * worldPos;
}