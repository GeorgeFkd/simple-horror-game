#version 330 core

// these locations must match the glVertexAttribPointer calls you made:
//   0 → position, 1 → texcoord, 2 → normal
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

// our only uniform for now: the combined View×Projection matrix
uniform mat4 uViewProj;

out vec2 vTexCoord;
out vec3 vNormal;

void main() {
    // forward the UVs and normals to the fragment shader
    vTexCoord = aTexCoord;
    vNormal   = aNormal;

    // transform into clip-space:
    gl_Position = uViewProj * vec4(aPos, 1.0);
}