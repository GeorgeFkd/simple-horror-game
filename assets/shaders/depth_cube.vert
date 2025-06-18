#version 330 core
layout (location = 0) in vec3 aPos;

layout(location = 4) in vec4 iModelCol0;
layout(location = 5) in vec4 iModelCol1;
layout(location = 6) in vec4 iModelCol2;
layout(location = 7) in vec4 iModelCol3;

uniform mat4 uModel;
uniform bool uUseInstancing;

void main()
{
    mat4 modelMatrix = uUseInstancing
        ? mat4(iModelCol0, iModelCol1, iModelCol2, iModelCol3)
        : uModel;

    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);

    gl_Position = worldPos;
} 