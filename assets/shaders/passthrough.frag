#version 330 core

in  vec2 vTexCoord;
in  vec3 vNormal;

out vec4 FragColor;

void main() {
    // normalize and remap [-1,1]→[0,1] so you can actually see it as color
    vec3 n = normalize(vNormal) * 0.5 + 0.5;
    FragColor = vec4(n, 1.0);
}
