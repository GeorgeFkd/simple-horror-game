#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec4 aTangent;
layout(location = 4) in vec4 iModelCol0;
layout(location = 5) in vec4 iModelCol1;
layout(location = 6) in vec4 iModelCol2;
layout(location = 7) in vec4 iModelCol3;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;
uniform bool uUseInstancing;


out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3   TBN;

void main() {
    mat4 modelMatrix = uUseInstancing
        ? mat4(iModelCol0, iModelCol1, iModelCol2, iModelCol3)
        : uModel;
    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    FragPos       = worldPos.xyz;
    Normal        = normalMatrix * aNormal;
    vec3 N        = normalize(Normal);
    vec3 T        = normalize(normalMatrix * aTangent.xyz);
    vec3 B        = cross(N, T) * aTangent.w;
    TBN           = mat3(T, B, N);
    TexCoord      = aTexCoord;
    gl_Position   = uProj * uView * worldPos;
}