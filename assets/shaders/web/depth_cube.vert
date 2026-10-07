#version 300 es
// WebGL2 version of depth_cube.vert + depth_cube.geom: without geometry shaders each cube face is
// rendered in its own pass, with that face's view-projection matrix in shadowMatrix.
layout (location = 0) in vec3 aPos;

layout(location = 4) in vec4 iModelCol0;
layout(location = 5) in vec4 iModelCol1;
layout(location = 6) in vec4 iModelCol2;
layout(location = 7) in vec4 iModelCol3;

uniform mat4 uModel;
uniform bool uUseInstancing;
uniform mat4 shadowMatrix;

out vec4 FragPos;

void main()
{
    mat4 modelMatrix = uUseInstancing
        ? mat4(iModelCol0, iModelCol1, iModelCol2, iModelCol3)
        : uModel;

    FragPos     = modelMatrix * vec4(aPos, 1.0);
    gl_Position = shadowMatrix * FragPos;
}
