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
    bool useBumpMap;
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
    mat4 view;
    mat4 proj;
    float nearPlane;
    float farPlane;
    float power;
    vec3 color;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
    float attenuation_power;
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
uniform sampler2D   bumpMap;
uniform float       bumpScale; 

// one sampler2D per light (only spot & dir. matter here)
uniform sampler2D  shadowMap0;
uniform sampler2D  shadowMap1;
uniform sampler2D  shadowMap2;
uniform sampler2D  shadowMap3;
uniform sampler2D  shadowMap4;
uniform sampler2D  shadowMap5;
uniform sampler2D  shadowMap6;
uniform sampler2D  shadowMap7;

uniform samplerCube  shadowMapCube0;
uniform samplerCube  shadowMapCube1;
uniform samplerCube  shadowMapCube2;
uniform samplerCube  shadowMapCube3;
uniform samplerCube  shadowMapCube4;
uniform samplerCube  shadowMapCube5;
uniform samplerCube  shadowMapCube6;
uniform samplerCube  shadowMapCube7;

float LinearizeDepth(float depth, float nearPlane, float farPlane)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

// arrays of shadow‐map samplers
// uniform ;

//——————————————————————————————————————————————————————————————————————————
// per‐fragment inputs + output
//——————————————————————————————————————————————————————————————————————————

in  vec3  FragPos;
in  vec3  Normal;
in  vec2  TexCoord;
in  mat3  TBN;
out vec4  FragColor;

//——————————————————————————————————————————————————————————————————————————
// Poisson disk for PCF
// (https://github.com/iamkroot/shadow-mapping/blob/master/shaders/standard_frag.glsl)
//——————————————————————————————————————————————————————————————————————————

// Poisson Disk
vec2 poissonDisk[16] = vec2[](
vec2(-0.94201624, -0.39906216),
vec2(0.94558609, -0.76890725),
vec2(-0.094184101, -0.92938870),
vec2(0.34495938, 0.29387760),
vec2(-0.91588581, 0.45771432),
vec2(-0.81544232, -0.87912464),
vec2(-0.38277543, 0.27676845),
vec2(0.97484398, 0.75648379),
vec2(0.44323325, -0.97511554),
vec2(0.53742981, -0.47373420),
vec2(-0.26496911, -0.41893023),
vec2(0.79197514, 0.19090188),
vec2(-0.24188840, 0.99706507),
vec2(-0.81409955, 0.91437590),
vec2(0.19984126, 0.78641367),
vec2(0.14383161, -0.14100790)
);

// Method to get the degree of visibility of a fragment
float getVisibility(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D shadowMap) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    FragColor = vec4(vec3(fragPosLightSpace.z), 1);
//
    // normalize to [0,1] range
    // full formula: (((far-near) * coord) + near + far) / 2.0
    // we have far = 1, near = 0
    projCoords = projCoords * 0.5 + 0.5;
//
    // declare a bias to deal with shadow acne
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = clamp(0.0005 * tan(acos(cosTheta)), 0, 0.01);
    projCoords.z -= bias;
    float visibility = 1.0;
    float spreadParam = 500.0;
    for (int i = 0; i < 16; i++) {
        if (texture(shadowMap, projCoords.xy + poissonDisk[i] / spreadParam).r < projCoords.z){
            visibility -= 0.05;
        }
    }
    return visibility;
}

float getVisibilityPointLight(
    vec3 fragPos, 
    vec3 lightPos, 
    samplerCube shadowMap, 
    float farPlane
){
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    // use the light to fragment vector to sample from the depth map    
    float closestDepth = texture(shadowMap, fragToLight).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= farPlane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // now test for shadows
    //float bias = 0.05; 
    //float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

    //float shadow  = 0.0;
    //float bias    = 0.05; 
    //float samples = 4.0;
    //float offset  = 0.1;
    float shadow = 0.0;
    float bias   = 0.15;
    int samples  = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = 0.05;
    //float diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0;  

    //for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    //{
    //    for(float y = -offset; y < offset; y += offset / (samples * 0.5))
    //    {
    //        for(float z = -offset; z < offset; z += offset / (samples * 0.5))
    //        {
    //            float closestDepth = texture(shadowMap, fragToLight + vec3(x, y, z)).r; 
    //            closestDepth *= farPlane;   // undo mapping [0;1]
    //            if(currentDepth - bias > closestDepth)
    //                shadow += 1.0;
    //        }
    //    }
    //}
    //shadow /= (samples * samples * samples);

    vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );   

    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= farPlane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
        }

    shadow /= float(samples);

    return 1 - shadow;
}

//float getVisibility(vec4 fragPosLightSpace, sampler2D shadowMap)
//{
//    // perform perspective divide
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//    // transform to [0,1] range
//    projCoords = projCoords * 0.5 + 0.5;
//    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
//    float closestDepth = texture(shadowMap, projCoords.xy).r; 
//    // get depth of current fragment from light's perspective
//    float currentDepth = projCoords.z;
//    // check whether current frag pos is in shadow
//    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;
//
//    return shadow;
//}  

//float getVisibility(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D shadowMap)
//{
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//    projCoords = projCoords * 0.5 + 0.5;
////
//    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
//        projCoords.y < 0.0 || projCoords.y > 1.0 ||
//        projCoords.z > 1.0)
//        return 1.0;
////
//    float currentDepth = projCoords.z;
//    float closestDepth = texture(shadowMap, projCoords.xy).r;
////
//    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
//    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
////
//    return 1.0 - shadow; // return visibility
//}

//float getVisibility(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2D shadowMap)
//{
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//    projCoords = projCoords * 0.5 + 0.5;
//
//    // Discard if outside the shadow map
//    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
//        projCoords.y < 0.0 || projCoords.y > 1.0 ||
//        projCoords.z > 1.0)
//        return 1.0;
//
//    float currentDepth = projCoords.z;
//
//    // Screen-space slope bias (derivative-based)
//    float bias = max(0.0015 * (1.0 - dot(normal, lightDir)), 0.0005);
//    float slopeScale = length(vec2(dFdx(currentDepth), dFdy(currentDepth)));
//    bias += slopeScale * 0.001; // you can tweak this constant
//
//    // PCF sampling
//    float shadow = 0.0;
//    float samples = 0.0;
//    float texelSize = 1.0 / textureSize(shadowMap, 0).x;
//
//    for (int x = -1; x <= 1; ++x) {
//        for (int y = -1; y <= 1; ++y) {
//            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
//            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
//            samples += 1.0;
//        }
//    }
//
//    shadow /= samples;
//    return 1.0 - shadow;
//}

vec3 fetchNormal(){
    if(!material.useBumpMap){
        return normalize(Normal);
    }

    vec2 tex = 1.0 / textureSize(bumpMap, 0);

    float tl = texture(bumpMap, TexCoord + tex * vec2(-1,  1)).r;
    float  l = texture(bumpMap, TexCoord + tex * vec2(-1,  0)).r;
    float bl = texture(bumpMap, TexCoord + tex * vec2(-1, -1)).r;

    float tr = texture(bumpMap, TexCoord + tex * vec2( 1,  1)).r;
    float  r = texture(bumpMap, TexCoord + tex * vec2( 1,  0)).r;
    float br = texture(bumpMap, TexCoord + tex * vec2( 1, -1)).r;

    float  t = texture(bumpMap, TexCoord + tex * vec2( 0,  1)).r;
    float  b = texture(bumpMap, TexCoord + tex * vec2( 0, -1)).r;

    float bu = (tr + 2.0*r + br) - (tl + 2.0*l + bl);   // ∂b/∂u
    float bv = (bl + 2.0*b + br) - (tl + 2.0*t + tr);   // ∂b/∂v
    bu *= bumpScale / 8.0;   // divide by 8 (sum of kernel weights)
    bv *= bumpScale / 8.0;


    vec3 nTS = normalize(vec3(-bu, -bv, 1.0));
    return normalize(TBN * nTS);
}
void main()
{
    // 1) sample or fallback
    vec3 Ka = useAmbientMap  ? texture(ambientMap,  TexCoord).rgb : material.ambient;
    vec3 Kd = useDiffuseMap  ? texture(diffuseMap,  TexCoord).rgb : material.diffuse;
    vec3 Ks = useSpecularMap ? texture(specularMap, TexCoord).rgb : material.specular;

    // 2) prepare
    vec3 N = fetchNormal();
    //vec3 debugColor = N * 0.5 + 0.5;  
    //FragColor = vec4(debugColor, 1.0);
    //return;
    vec3 V = normalize(viewPos - FragPos);

    vec3 ambientAccum = vec3(0.0);
    vec3 diffuseAccum = vec3(0.0);
    vec3 specAccum    = vec3(0.0);

    // 3) light loop
    for (int i = 0; i < numLights; ++i) {
        
        Light L = lights[i];

        
        // compute fragment position in light space
        vec4 fragPosLightSpace = L.proj * L.view * vec4(FragPos, 1.0);

        // compute light direction
        vec3 Ldir = (L.type == 1)
            ? normalize(-L.direction)               // directional
            : normalize(L.position - FragPos);      // point or spot

        // compute spotlight factor
        float spotFactor = 1.0;
        if (L.type == 2) {
            float theta   = dot(Ldir, normalize(-L.direction));
            float epsilon = L.cutoff - L.outerCutoff;
            spotFactor    = clamp((theta - L.outerCutoff) / epsilon, 0.0, 1.0);
        }
        
        // distance attenuation for non-directional lights
        float intensity = 1.0;
        //float intensity = 1;
        if (L.type != 1) {
            float d   = length(L.position - FragPos);
            // classic 1/(c + l·d + q·d²) attenuation:
            float atten = 1.0
                / (L.attenuation_constant
                + L.attenuation_linear   * d
                + L.attenuation_quadratic * d * d);
            intensity = pow(atten, L.attenuation_power) * L.power;
        }

        // calculate shadow visibility (only for spot & directional)
        float visibility = 1.0;
        if (L.type == 2) {
            if      (i == 0) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap0);
            else if (i == 1) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap1);
            else if (i == 2) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap2);
            else if (i == 3) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap3);
            else if (i == 4) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap4);
            else if (i == 5) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap5);
            else if (i == 6) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap6);
            else if (i == 7) visibility = getVisibility(fragPosLightSpace, Normal, Ldir, shadowMap7);
        }
        else if(L.type == 0){
            //if      (i == 0) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube0, L.farPlane);
            //else if (i == 1) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube1, L.farPlane);
            if (i == 2) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube2, L.farPlane);
            //else if (i == 3) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube3, L.farPlane);
            //else if (i == 4) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube4, L.farPlane);
            //else if (i == 5) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube5, L.farPlane);
            //else if (i == 6) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube6, L.farPlane);
            //else if (i == 7) visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube7, L.farPlane);
        }

        //visibility = getVisibilityPointLight(FragPos, L.position, shadowMapCube0, L.farPlane);
        //FragColor = vec4(vec3(visibility), 1.0);
        //return;
        // ambient term (ambient unaffected by shadows)
        ambientAccum += visibility * intensity * spotFactor * Ka;

        // diffuse term (modulated by shadow visibility)
        float diff = max(dot(N, Ldir), 0.0);
        diffuseAccum += visibility * intensity * spotFactor * L.color * Kd * diff;

        //FragColor = vec4(diffuseAccum, 1.0);
        //return;

        // specular term (modulated by shadow visibility)
        vec3 H    = normalize(Ldir + V);
        float spec = pow(max(dot(N, H), 0.0), material.shininess);
        specAccum += visibility * intensity * spotFactor * L.color * Ks * spec;

        //FragColor = vec4(specAccum * Ks, 1.0);
        //return;

        //FragColor = vec4(specAccum, 1.0);
        //return FragColor;
    }

    // 4) combine
    vec3 color = material.emissive
               + ambientAccum * 0.05
               + diffuseAccum
               + specAccum * 0.1;


    //vec3 color = material.emissive
    //           + ambientAccum
    //           + diffuseAccum
    //           + specAccum;

    //vec4 fragPosLightSpace = lights[0].proj * lights[0].view * vec4(FragPos, 1.0);
    //vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    //projCoords = projCoords * 0.5 + 0.5;
    //float depth = texture(shadowMap0, projCoords.xy).r;
    ////depth -= 0.05;
    ////float depth = projCoords.z;
    //float linear = LinearizeDepth(depth, lights[0].nearPlane, lights[0].farPlane) / lights[0].farPlane;
    ////FragColor = vec4(depth);
    //FragColor = vec4(vec3(linear), 1.0); // Grayscale
    //// Only valid if within [0,1] bounds
    //if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    //    projCoords.y < 0.0 || projCoords.y > 1.0 ||
    //    projCoords.z > 1.0)
    //{
    //    FragColor = vec4(1.0, 0.0, 1.0, 1.0); // outside light frustum
    //}
    //vec3 fragToLight = FragPos - lights[2].position;

    //float linearDepth = length(fragToLight) / lights[2].farPlane;
    //FragColor = vec4(vec3(linearDepth), 1.0);
    //return;

    // --- DEBUG: visualize the stored shadow‐map depth ---
    //vec3 fragToLight = FragPos - lights[2].position;
    ////fragToLight = FragPos - lights[2].position;

    ////make sure you use the *same* index for sampler & farPlane:
    //float storedDepth = texture(shadowMapCube2,  fragToLight).r;   
    ////it's already in [0,1], but if you need real distance:
    ////storedDepth *= lights[2].farPlane;

    //FragColor = vec4(vec3(storedDepth), 1.0);
    //return;
    FragColor = vec4(color, material.opacity);
}
