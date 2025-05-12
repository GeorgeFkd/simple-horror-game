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

uniform Material material;
uniform Light lights[MAX_LIGHTS];
uniform int   numLights;
uniform vec3  viewPos;

// shadow map samplers
uniform sampler2D  shadowMap[MAX_LIGHTS];
uniform samplerCube shadowCube[MAX_LIGHTS];
// light-space matrices for dir/spot lights
uniform mat4       lightSpace[MAX_LIGHTS];

in  vec3 FragPos;
in  vec3 Normal;
in  vec2 TexCoord;
out vec4 FragColor;

// helper: compute whether this fragment is in shadow for light i
float calcShadow(int i) {
    if (lights[i].type == 0) {
        // point light: compare radial distance against cube map
        float dist    = length(FragPos - lights[i].position);
        float closest = texture(shadowCube[i], FragPos - lights[i].position).r;
        float bias    = 0.05;
        return (dist - bias > closest) ? 1.0 : 0.0;
    } else {
        // directional or spot light
        vec4 projCoords = lightSpace[i] * vec4(FragPos, 1.0);
        // perform perspective divide and transform to [0,1]
        vec3 coords = projCoords.xyz / projCoords.w * 0.5 + 0.5;
        // if outside the shadow map, consider lit
        if (coords.x < 0.0 || coords.x > 1.0 ||
            coords.y < 0.0 || coords.y > 1.0 ||
            coords.z < 0.0 || coords.z > 1.0) {
            return 0.0;
        }
        float closest = texture(shadowMap[i], coords.xy).r;
        float current = coords.z;
        float bias    = 0.005;
        return (current - bias > closest) ? 1.0 : 0.0;
    }
}

void main() {
    // start with emissive term
    vec3 result = material.emissive;

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 ambAccum  = vec3(0.0);
    vec3 diffAccum = vec3(0.0);
    vec3 specAccum = vec3(0.0);

    // accumulate lighting from each light
    for (int i = 0; i < numLights; ++i) {
        Light L = lights[i];

        // ambient
        ambAccum += L.ambient * material.ambient;

        // compute light direction
        vec3 Ldir;
        if (L.type == 1) {
            // directional light: direction is from light into scene
            Ldir = normalize(-L.direction);
        } else {
            // point or spot: from fragment toward light position
            Ldir = normalize(L.position - FragPos);
        }

        // diffuse (Lambert)
        float diff = max(dot(norm, Ldir), 0.0);

        // specular (Blinn–Phong)
        vec3 halfway = normalize(Ldir + viewDir);
        float spec   = pow(max(dot(norm, halfway), 0.0), material.shininess);

        // spotlight cone attenuation
        float intensity = 1.0;
        if (L.type == 2) {
            float theta   = dot(Ldir, normalize(-L.direction));
            float epsilon = L.cutoff - L.outerCutoff;
            intensity = clamp((theta - L.outerCutoff) / epsilon, 0.0, 1.0);
        }

        // shadow factor (0 = fully lit, 1 = fully in shadow)
        float shadow = calcShadow(i);

        // accumulate, modulated by shadow and intensity
        diffAccum += (1.0 - shadow) * intensity * diff * L.diffuse  * material.diffuse;
        specAccum += (1.0 - shadow) * intensity * spec * L.specular * material.specular;
    }

    // combine based on illumModel
    if (material.illumModel == 0) {
        // ambient only
        result += ambAccum;
    }
    else if (material.illumModel == 1) {
        // ambient + diffuse
        result += ambAccum + diffAccum;
    }
    else {
        // full Blinn–Phong
        result += ambAccum + diffAccum + specAccum;
    }

    FragColor = vec4(result, material.opacity);
}
