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


layout(push_constant) uniform constant
{
    int mode;
} view_mode;



float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
{
	// TODO: Maybe this needs to be in a vertex shader?
	vec4 fragPosLightSpace = sun.viewProj * vec4(fragPos, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	//projCoords.y = -projCoords.y;
	projCoords = projCoords * 0.5 + 0.5;

	float shadowDepth = texture(samplerShadowDepth, projCoords.xy).r;
	float currentDepth = projCoords.z;

	// Check if current pixel is in shadow
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
	float shadow = currentDepth - bias > shadowDepth ? 1.0 : 0.0;
    
	return shadow;	
}

void main()
{
	// textures
	vec3 world_pos = texture(samplerPosition, inUV).rgb;
	vec3 normal = normalize(texture(samplerNormal, inUV).rgb);
	vec3 albedo = texture(samplerAlbedo, inUV).rgb;
	float spec = texture(samplerSpecular, inUV).r;
	float depth = texture(samplerDepth, inUV).r;
	float shadowDepth = texture(samplerShadowDepth, inUV).r;

	// For debugging purposes
	if (view_mode.mode > 0) {
		vec3 debug_color = vec3(0.0);
		switch (view_mode.mode) {
			case 1:
				debug_color = albedo;
				break;
			case 2:
				debug_color = vec3(spec);
				break;
			case 3:
				debug_color = world_pos;
				break;
			case 4:
				debug_color = normal;
				break;
			case 5:
				debug_color = depth.rrr;
				break;
			case 6:
				debug_color = shadowDepth.rrr;
				break;
		}

		outFragcolor = vec4(debug_color, 1.0);
		return; 
	}





	// constants
	vec3 camera_pos = scene.cameraPosition.xyz;



	//////////////// AMBIENT ////////////////
	float ambient_strength = scene.ambientSpecular.r;
	vec3 ambient = vec3(ambient_strength);


	//////////////// DIFFUSE ////////////////
	vec3 light_dir = normalize(-scene.sunDirection);
	float diffuse_factor = max(dot(normal, light_dir), 0.0);
	vec3 diffuse = vec3(diffuse_factor);

	//////////////// SPECULAR ////////////////
	vec3 camera_dir = normalize(camera_pos - world_pos);
	vec3 halfway_dir = normalize(camera_dir + light_dir);
	//vec3 light_reflect_dir = reflect(-scene.sunDirection, normal);

	vec3 specularReflect = reflect(-light_dir, normal);
	float specFactor = max(0.0, dot(specularReflect, camera_dir));
	vec3 specular = vec3(pow(specFactor, scene.ambientSpecular.b));

	float shadow = ShadowCalculation(world_pos, normal, light_dir);

	vec3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * albedo;
	outFragcolor = vec4(result, 1.0);
}