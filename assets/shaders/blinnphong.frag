#version 330 core

#define MAX_LIGHTS 8

struct Material {
    vec3 ambient;      // Ka
    vec3 diffuse;      // Kd
    vec3 specular;     // Ks
    vec3 emissive;     // Ke
    float shininess;   // Ns
    float opacity;     // d
    int   illumModel;  // illum
    float ior;         // Ni
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
    int   type;         // 0=point,1=dir,2=spot
};
uniform Light lights[MAX_LIGHTS];
uniform int   numLights;

uniform vec3 viewPos;

in  vec3 FragPos;
in  vec3 Normal;
in  vec2 TexCoord;
out vec4 FragColor;

void main() {
    // start with emissive term
    vec3 result = material.emissive;

    // compute per-light diffuse & specular
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 diffAccum = vec3(0.0);
    vec3 specAccum = vec3(0.0);

    for (int i = 0; i < numLights; ++i) {
        Light L = lights[i];
        vec3 Ldir;
        if (L.type == 1) {
            Ldir = normalize(-L.direction);
        } else {
            Ldir = normalize(L.position - FragPos);
        }

        float diff  = max(dot(norm, Ldir), 0.0);
        vec3 halfway = normalize(Ldir + viewDir);
        float spec  = pow(max(dot(norm, halfway), 0.0), material.shininess);

        // spotlight cone:
        float intensity = 1.0;
        if (L.type == 2) {
            float theta   = dot(Ldir, normalize(-L.direction));
            float epsilon = L.cutoff - L.outerCutoff;
            intensity = clamp((theta - L.outerCutoff) / epsilon, 0.0, 1.0);
        }

        diffAccum += intensity * diff * L.diffuse  * material.diffuse;
        specAccum += intensity * spec * L.specular * material.specular;
    }

    // ambient is always applied (depending on illumModel)
    vec3 amb = material.ambient * lights[0].ambient;  
    // if there are multiple light ambients you can sum them too

    // combine based on illumModel
    if (material.illumModel == 0) {
        // ambient only
        result += amb;
    }
    else if (material.illumModel == 1) {
        // ambient + diffuse
        result += amb + diffAccum;
    }
    else {
        // full Blinn–Phong
        result += amb + diffAccum + specAccum;
    }

    // (optionally can use material.ior here for a simple Fresnel:)
    // float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), material.ior);
    // result = mix(result, specAccum, fresnel);

    FragColor = vec4(result, material.opacity);
}
