#version 330 core

#define MAX_LIGHTS 8

//——————————————————————————————————————————————————————————————————————————
// material + light structs
//——————————————————————————————————————————————————————————————————————————

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
    mat4  lightSpaceMatrix;
    //sampler2D   shadowMap2D;
};

uniform Material    material;
uniform Light       lights[MAX_LIGHTS];
uniform int         numLights;
uniform vec3        viewPos;


//——————————————————————————————————————————————————————————————————————————
// material maps + toggles
//——————————————————————————————————————————————————————————————————————————

uniform sampler2D   ambientMap;
uniform bool        useAmbientMap;
uniform sampler2D   diffuseMap;
uniform bool        useDiffuseMap;
uniform sampler2D   specularMap;
uniform bool        useSpecularMap;

// arrays of shadow‐map samplers
// uniform ;

//——————————————————————————————————————————————————————————————————————————
// per‐fragment inputs + output
//——————————————————————————————————————————————————————————————————————————

in  vec3  FragPos;
in  vec3  Normal;
in  vec2  TexCoord;
out vec4  FragColor;

//——————————————————————————————————————————————————————————————————————————
// Poisson disk for PCF
// (https://github.com/iamkroot/shadow-mapping/blob/master/shaders/standard_frag.glsl)
//——————————————————————————————————————————————————————————————————————————

vec2 poissonDisk[16] = vec2[]( 
    vec2(-0.94201624, -0.39906216),
    vec2( 0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),
    vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554),
    vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),
    vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367),
    vec2( 0.14383161, -0.14100790)
);

//——————————————————————————————————————————————————————————————————————————
// Method to get the degree of visibility of a fragment
// (soft shadows via PCF)
//——————————————————————————————————————————————————————————————————————————

float getVisibility(
    vec4   fragPosLightSpace,
    vec3   normal,
    vec3   lightDir,
    sampler2D shadowMap
) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // normalize to [0,1] range
    // full formula: (((far-near) * coord) + near + far) / 2.0
    // we have far = 1, near = 0
    projCoords = projCoords * 0.5 + 0.5;

    // declare a bias to deal with shadow acne
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias     = clamp(0.0005 * tan(acos(cosTheta)), 0.0, 0.01);
    projCoords.z  -= bias;

    float visibility   = 1.0;
    float spreadParam  = 500.0;
    for (int i = 0; i < 16; ++i) {
        if (texture(shadowMap, projCoords.xy + poissonDisk[i] / spreadParam).r 
            < projCoords.z) 
        {
            visibility -= 0.05;
        }
    }
    return visibility;
}

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
    for (int i = 0; i < numLights; ++i) {
        Light L = lights[i];

        // compute fragment position in light space
        vec4 fragPosLightSpace = L.lightSpaceMatrix * vec4(FragPos, 1.0);

        // compute light direction
        vec3 Ldir = (L.type == 1)
            ? normalize(-L.direction)               // directional
            : normalize(L.position - FragPos);      // point or spot

        // compute spotlight factor
        float spotFactor = 1.0;
        if (L.type == 2) {
            float theta   = dot(Ldir, normalize(-L.direction));
            float epsilon = L.cutoff - L.outerCutoff;
            spotFactor     = clamp((theta - L.outerCutoff) / epsilon, 0.0, 1.0);
        }

        // distance attenuation for non-directional lights
        float intensity = 1.0;
        if (L.type != 1) {
            float distance = length(L.position - FragPos);
            intensity      = (1.0 / (0.3 * distance)) * 0.8;
        }

        // calculate shadow visibility (only for spot & directional)
        float visibility = 1.0;
        //if (L.type != 0) {
        //    visibility = getVisibility(
        //        fragPosLightSpace,
        //        N,
        //        Ldir,
        //        shadowMap2D[i]
        //    );
        //}

        // ambient term (ambient unaffected by shadows)
        ambientAccum += intensity * spotFactor * L.ambient * Ka;

        // diffuse term (modulated by shadow visibility)
        float diff = max(dot(N, Ldir), 0.0);
        diffuseAccum += visibility * intensity * spotFactor * L.diffuse * Kd * diff;

        // specular term (modulated by shadow visibility)
        vec3 H    = normalize(Ldir + V);
        float spec = pow(max(dot(N, H), 0.0), material.shininess);
        specAccum += visibility * intensity * spotFactor * L.specular * Ks * spec;
    }

    // 4) combine
    vec3 color = material.emissive
               + ambientAccum * 0.4
               + diffuseAccum * 0.8
               + specAccum    * 0.1;

    FragColor = vec4(color, material.opacity);
}
