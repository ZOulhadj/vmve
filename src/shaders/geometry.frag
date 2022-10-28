#version 450

layout(location = 0) out vec4 final_color;

layout(location = 1) in vec2 texture_coord;
layout(location = 2) in vec3 vertex_position;
//layout(location = 3) in vec3 vertex_normal;

layout(location = 4) in vec3 tangentLightPos;
layout(location = 5) in vec3 tangentViewPos;
layout(location = 6) in vec3 tangentFragPos;

layout(binding = 1) uniform scene_ubo
{
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    vec3 cameraPosition;
    vec3 lightPosition;
    vec3 lightColor;
} scene;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D specularTexture;

void main()
{

    // Get pixel colors for each texture map
    vec3 albedo   = texture(albedoTexture, texture_coord).rgb;
    vec3 normal   = texture(normalTexture, texture_coord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    vec3 specular = texture(specularTexture, texture_coord).rgb;


    // Global properties
    vec3 normalized_normal = normalize(normal);
    vec3 lightDir   = normalize(tangentLightPos - tangentFragPos);
    vec3 viewDir    = normalize(tangentViewPos - tangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normalized_normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);



    // ambient
    vec3 ambientResult = scene.ambientStrength * albedo;

    // diffuse
    vec3 diffuseResult = max(dot(normalized_normal, lightDir), 0.0) * albedo;

    // specular
    float spec          = pow(max(dot(viewDir, halfwayDir), 0.0), scene.specularShininess);
    vec3 specularResult = scene.specularStrength * spec * specular;


    // final result
    vec3 result = ambientResult + diffuseResult + specularResult;
    final_color = vec4(diffuseResult, 1.0);
}