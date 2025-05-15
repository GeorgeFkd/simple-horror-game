#version 330 core

#define MAX_LIGHTS 8

struct Material {
    vec3 ambient;      // Ka (fallback)
    vec3 diffuse;      // Kd
    vec3 specular;     // Ks
    vec3 emissive;     // Ke
    float shininess;   // Ns
    float opacity;     // d
};

struct Light {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float cutoff;      // inner cone cosine (spot only)
    float outerCutoff; // outer cone cosine (spot only)
    int   type;        // 0=point,1=directional,2=spot
};

uniform Material  material;
uniform Light     lights[MAX_LIGHTS];
uniform int       numLights;
uniform vec3      viewPos;

// material maps + toggles
uniform sampler2D ambientMap;
uniform bool      useAmbientMap;
uniform sampler2D diffuseMap;
uniform bool      useDiffuseMap;
uniform sampler2D specularMap;
uniform bool      useSpecularMap;

in  vec3  FragPos;
in  vec3  Normal;
in  vec2  TexCoord;
out vec4 FragColor;

void main()
{
    // 1) sample or fallback
    vec3 Ka = useAmbientMap  ? texture(ambientMap,  TexCoord).rgb : material.ambient;
    vec3 Kd = useDiffuseMap  ? texture(diffuseMap,  TexCoord).rgb : material.diffuse;
    vec3 Ks = useSpecularMap ? texture(specularMap, TexCoord).rgb : material.specular;

    // 2) prepare
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 ambientAccum = vec3(0.0);
    vec3 diffuseAccum = vec3(0.0);
    vec3 specAccum    = vec3(0.0);

    // 3) light loop
    for(int i = 0; i < numLights; ++i) {
        Light L = lights[i];

        // compute light direction
        vec3 Ldir;
        if (L.type == 1) {
            // directional
            Ldir = normalize(-L.direction);
        } else {
            // point or spot
            Ldir = normalize(L.position - FragPos);
        }

        // compute spotlight factor
        float spotFactor = 1.0;
        if (L.type == 2) {
            float theta = dot(Ldir, normalize(-L.direction));
            float epsilon = L.cutoff - L.outerCutoff;
            spotFactor = clamp((theta - L.outerCutoff) / epsilon, 0.0, 1.0);
        }

        // ambient term
        ambientAccum += spotFactor * L.ambient * Ka;

        // diffuse term
        float diff = max(dot(N, Ldir), 0.0);
        diffuseAccum += spotFactor * L.diffuse * Kd * diff;

        // specular term (Blinn-Phong)
        vec3 H = normalize(Ldir + V);
        float spec = pow(max(dot(N, H), 0.0), material.shininess);
        specAccum += spotFactor * L.specular * Ks * spec;
    }

    // 4) combine
    vec3 color = material.emissive * 0.4f
               + ambientAccum * 0.5f
               + diffuseAccum
               + specAccum * 0.1f;

    FragColor = vec4(color, material.opacity);
}
