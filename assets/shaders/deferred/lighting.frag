#version 450



//
// Geometry Uniform Buffers
// 
layout (binding = 0) uniform sampler2D samplerPosition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerDepth;

layout(binding = 4) uniform scene_ubo
{
    // ambient Strength, specular strength, specular shininess, empty
    vec4 ambientSpecular;
    vec4 cameraPosition;

    // light position (x, y, z), light strength
    vec4 lightPosStrength;
} scene;


//
// Material Uniform Buffers
//
//layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
//layout(set = 1, binding = 1) uniform sampler2D normalTexture;
//layout(set = 1, binding = 2) uniform sampler2D specularTexture;
//
//


layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;


layout(push_constant) uniform constant
{
    int mode;
} view_mode;


void main()
{

	// framebuffers
	vec3 position = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec3 color = texture(samplerAlbedo, inUV).rgb;
	float depth = texture(samplerDepth, inUV).r;

	float ambient_strength = scene.ambientSpecular.r;

	vec3 light_position = scene.lightPosStrength.xyz;
	float light_strength = scene.lightPosStrength.w;


	vec3 ambient = ambient_strength * color * light_strength;



    vec3 viewDir    = normalize(light_position - position);
    vec3 lightDir   = normalize(light_position - position);
    vec3 diffuse = max(dot(lightDir, normal), 0.0) * color * light_strength;




	vec3 final_color = vec3(0.0);
	if (view_mode.mode == 0) {
		final_color = vec3(ambient + diffuse);
	} else if (view_mode.mode == 1) {
		final_color = color;
	} else if (view_mode.mode == 2) {
		final_color = position;
	} else if (view_mode.mode == 3) {
		final_color = normal;
	} else if (view_mode.mode == 4) {
		final_color = vec3(depth);
	} 

	outFragcolor = vec4(final_color, 1.0);

















//
//	vec3 albedo = texture(samplerAlbedo, texture_coord).rgb;
//    vec3 normal = texture(samplerNormal, texture_coord).rgb;
//    normal = normalize(normal);
//    vec3 specular = texture(specularTexture, texture_coord).rgb;
//
//
//    // ambient
//    vec3 ambientResult = scene.ambientStrength * albedo * scene.lightColor;
//
//    // diffuse
//    vec3 viewDir    = normalize(tangentViewPos - tangentFragPos);
//    vec3 lightDir   = normalize(tangentLightPos - tangentFragPos);
//
//    vec3 diffuseResult = max(dot(lightDir, normal), 0.0) * albedo * scene.lightColor;
//
//    // specular
//    //vec3 reflectDir = reflect(-lightDir, normal);
//    vec3 halfwayDir = normalize(lightDir + viewDir);
//
//    float spec = pow(max(dot(normal, halfwayDir), 0.0), scene.specularShininess);
//    vec3 specularResult  = spec * specular;
//
//    vec3 result = ambientResult + diffuseResult + specularResult;
//
//    final_color = vec4(result, 1.0);
}