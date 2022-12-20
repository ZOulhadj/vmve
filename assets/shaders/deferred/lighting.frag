#version 450



//
// Geometry Uniform Buffers
// 
layout (binding = 0) uniform sampler2D samplerPosition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerSpecular;
layout (binding = 4) uniform sampler2D samplerDepth;

layout(binding = 5) uniform scene_ubo
{
    // ambient Strength, specular strength, specular shininess, empty
    vec4 ambientSpecular;
    vec4 cameraPosition;

    // light position (x, y, z), light strength
    vec4 lightPosStrength;
} scene;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;


layout(push_constant) uniform constant
{
    int mode;
} view_mode;


void main()
{
	// textures
	vec3 world_pos = texture(samplerPosition, inUV).rgb;
	vec3 normal = normalize(texture(samplerNormal, inUV).rgb);
	vec3 albedo = texture(samplerAlbedo, inUV).rgb;
	float spec = texture(samplerSpecular, inUV).r;
	float depth = texture(samplerDepth, inUV).r;

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
				debug_color = vec3(depth);
				break;
		}

		outFragcolor = vec4(debug_color, 1.0);
		return; 
	}





	// constants
	vec3 camera_pos = scene.cameraPosition.xyz;
	vec3 light_pos = scene.lightPosStrength.xyz;
	float light_strength = scene.lightPosStrength.w;


	//////////////// AMBIENT ////////////////
	float ambient_strength = scene.ambientSpecular.r;
	vec3 ambient = albedo * ambient_strength;

	//////////////// DIFFUSE ////////////////
	vec3 light_dir = normalize(light_pos - world_pos);
	float light_diff = max(0.0, dot(normal, light_dir));
    vec3 diffuse = albedo * light_diff;

	//////////////// SPECULAR ////////////////
	vec3 light_reflect_dir = reflect(-light_dir, normal);
	vec3 camera_dir = normalize(camera_pos - world_pos);

	vec3 halfway_dir = normalize(camera_dir + light_dir);
	float specular_strength = max(0.0, dot(light_reflect_dir, camera_dir));
	vec3 specular = vec3(spec) * pow(specular_strength, scene.ambientSpecular.b);

	vec3 final_color = ambient + diffuse + specular;
	outFragcolor = vec4(final_color, 1.0);
}