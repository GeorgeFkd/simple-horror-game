// assets/shaders/blinnphong.frag
#version 330 core

#define MAX_LIGHTS 8

struct Material {
    vec3 ambient;    // Ka
    vec3 diffuse;    // Kd
    vec3 specular;   // Ks
    float shininess; // Ns
};
uniform Material material;

struct Light {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float cutoff;       // inner cone cosine
    float outerCutoff;  // outer cone cosine
    int type;           // 0 = point, 1 = directional, 2 = spotlight
};
uniform Light lights[MAX_LIGHTS];
uniform int   numLights;

uniform vec3 viewPos;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

void main() {
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 color   = vec3(0.0);

    for (int i = 0; i < numLights; ++i) {
        Light L = lights[i];

        vec3 lightDir;
        float attenuation = 1.0;

        if (L.type == 1) {
            // Directional light: direction is already normalized
            lightDir = normalize(-L.direction);
        } else {
            // Point or spot
            lightDir = normalize(L.position - FragPos);
            // Optionally add distance‐based attenuation here
        }

        // Diffuse term
        float diff = max(dot(norm, lightDir), 0.0);

        // Specular (Blinn–Phong)
        vec3 halfway = normalize(lightDir + viewDir);
        float spec  = pow(max(dot(norm, halfway), 0.0), material.shininess);

        // Spotlight intensity
        float intensity = 1.0;
        if (L.type == 2) {
            float theta = dot(lightDir, normalize(-L.direction));
            float epsilon = L.cutoff - L.outerCutoff;
            intensity = clamp((theta - L.outerCutoff) / epsilon, 0.0, 1.0);
        }

        vec3 ambient  = L.ambient  * material.ambient;
        vec3 diffuse  = L.diffuse  * diff * material.diffuse;
        vec3 specular = L.specular * spec * material.specular;

        color += intensity * (ambient + diffuse + specular) * attenuation;
    }

    FragColor = vec4(color, 1.0);
}
