#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

// these four locations must match how you set up your instance VBO
layout(location = 3) in vec4 iModelCol0;
layout(location = 4) in vec4 iModelCol1;
layout(location = 5) in vec4 iModelCol2;
layout(location = 6) in vec4 iModelCol3;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;
uniform bool uUseInstancing;


out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    mat4 modelMatrix = uUseInstancing
        ? mat4(iModelCol0, iModelCol1, iModelCol2, iModelCol3)
        : uModel;

    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    FragPos       = worldPos.xyz;
    Normal        = mat3(transpose(inverse(modelMatrix))) * aNormal;
    TexCoord      = aTexCoord;
    gl_Position   = uProj * uView * worldPos;
}
