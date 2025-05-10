#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

// material
uniform sampler2D uDiffuse;    // color map (if you have one)
uniform vec3      uKa;         // ambient reflectivity
uniform vec3      uKd;         // diffuse reflectivity
uniform vec3      uKs;         // specular reflectivity
uniform float     uShininess;  // specular exponent

// light
uniform vec3 uLightPos;
uniform vec3 uLightColor;

// camera
uniform vec3 uViewPos;

void main() {
    // ambient
    vec3 ambient = uKa * uLightColor;

    // diffuse
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = uKd * diff * uLightColor;

    // specular (Blinn-Phong: halfway vector)
    vec3 V = normalize(uViewPos - vFragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess);
    vec3 specular = uKs * spec * uLightColor;

    // texture
    vec3 texColor = texture(uDiffuse, vTexCoord).rgb;

    // combine
    vec3 result = (ambient + diffuse + specular) * texColor;
    FragColor = vec4(result, 1.0);
}
