#version 450



//
// Geometry Uniform Buffers
// 
layout (binding = 0) uniform sampler2D samplerPosition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerSpecular;
layout (binding = 4) uniform sampler2D samplerDepth;
layout (binding = 5) uniform sampler2D samplerShadowDepth;
layout (binding = 6) uniform SunData 
{
	mat4 viewProj;
} sun;

layout(binding = 7) uniform scene_ubo
{
    // ambient Strength, specular strength, specular shininess, empty
    vec4 ambientSpecular;
    vec4 cameraPosition;

	vec3 sunDirection;
	vec3 sunPosition;
} scene;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

float ShadowCalculation(vec3 fragPos) 
{
	// TODO: Maybe this needs to be in a vertex shader?
	vec4 fragPosLightSpace = sun.viewProj * vec4(fragPos, 1.0);

    vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;

	float shadowDepth = texture(samplerShadowDepth, projCoords.xy).r;
	float currentDepth = projCoords.z;

	// Check if current pixel is in shadow
	//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
	float shadow = currentDepth > shadowDepth ? 1.0 : 0.0;

	return shadowDepth;	
}

void main()
{
	// textures
	vec3 world_pos = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec3 albedo = texture(samplerAlbedo, inUV).rgb;
	float spec = texture(samplerSpecular, inUV).r;
	float depth = texture(samplerDepth, inUV).r;
	
	// TODO: Maybe this needs to be in a vertex shader?
	vec4 fragPosLightSpace = sun.viewProj * vec4(world_pos, 1.0);
    vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;
	float shadowDepth = texture(samplerShadowDepth, projCoords.xy).r;

	vec3 lightColor = vec3(1.0);

	//////////////// AMBIENT ////////////////
	vec3 ambient = scene.ambientSpecular.rrr * lightColor;


	//////////////// DIFFUSE ////////////////
	vec3 light_dir = normalize(scene.sunPosition - world_pos);
	float diffuse_factor = max(dot(light_dir, normal), 0.0);
	vec3 diffuse = diffuse_factor * lightColor;

	//////////////// SPECULAR ////////////////
	vec3 camera_dir = normalize(scene.cameraPosition.xyz - world_pos);
	vec3 halfway_dir = normalize(camera_dir + light_dir);
	//vec3 light_reflect_dir = reflect(-scene.sunDirection, normal);

	float specFactor = max(dot(normal, halfway_dir), 0.0);
	vec3 specular = pow(specFactor, scene.ambientSpecular.b) * lightColor;

	float shadow = ShadowCalculation(world_pos);

	vec3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * albedo;
	outFragcolor = vec4(result, 1.0);
}