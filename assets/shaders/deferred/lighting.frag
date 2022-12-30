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

	vec3 lightDirection;
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

	vec3 final_color = vec3(0.0);


	//////////////// AMBIENT ////////////////
	float ambient_strength = scene.ambientSpecular.r;
	vec3 ambient = albedo * ambient_strength;

	final_color += ambient;

	//////////////// DIFFUSE ////////////////
	vec3 light_dir = normalize(-scene.lightDirection);
	float diffuse_factor = dot(normal, light_dir);

	// If we are facing away from the light simply return and don't do any
	// additional lighting calculations
	if (diffuse_factor <= 0) {
		outFragcolor = vec4(final_color, 1.0);
		return;
	}


	// At this stage, we know that additional lighting calculations are needed
	// since we are facing the light.
    vec3 diffuse = albedo * diffuse_factor;
	final_color += diffuse;


	//////////////// SPECULAR ////////////////
	vec3 light_reflect_dir = reflect(scene.lightDirection, normal);
	vec3 camera_dir = normalize(camera_pos - world_pos);
	//vec3 halfway_dir = normalize(camera_dir + light_dir);

	float specular_factor = dot(light_reflect_dir, camera_dir);
	if (specular_factor <= 0) {
		outFragcolor = vec4(final_color, 1.0);
		return;
	}

	vec3 specular = vec3(spec) * pow(specular_factor, scene.ambientSpecular.b);
	final_color += specular;


	outFragcolor = vec4(final_color, 1.0);
}