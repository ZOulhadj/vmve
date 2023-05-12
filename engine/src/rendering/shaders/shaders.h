#ifndef MY_ENGINE_SHADERS_HPP
#define MY_ENGINE_SHADERS_HPP

const std::string geometry_vs_code = R"(
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;

layout(location = 0) out vec2 texture_coord;
layout(location = 1) out vec3 vertex_position;
layout(location = 2) out vec3 vertex_normal;
layout(location = 3) out vec3 vertex_tangent;


layout(binding = 0) uniform model_view_projection {
    mat4 view;
    mat4 proj;
} mvp;

layout(push_constant) uniform constant
{
    mat4 model;
} obj;

void main()
{
    // Transform vertex from model to projection space
    // Model space -> World Space -> View Space -> Projection Space
    gl_Position = mvp.proj * mvp.view * obj.model * vec4(position, 1.0);
   
    vertex_position = vec3(obj.model * vec4(position, 1.0));

    texture_coord = uv;

    // Calculate TBN matrix required for normal mapping
    mat3 M = transpose(inverse(mat3(obj.model)));
    vertex_tangent = M * normalize(tangent);
    vertex_normal = M * normalize(normal);
}
)";

const std::string geometry_fs_code = R"(
#version 450

layout(location = 0) in vec2 texture_coord;
layout(location = 1) in vec3 vertex_position;
layout(location = 2) in vec3 vertex_normal;
layout(location = 3) in vec3 vertex_tangent;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D specularTexture;

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_color;
layout(location = 3) out vec4 out_specular;

void main()
{
    out_position = vec4(vertex_position, 1.0);

    out_color = texture(albedoTexture, texture_coord);
    out_specular = texture(specularTexture, texture_coord);   

    // Create the bitangent
    vec3 T = normalize(vertex_tangent);
    vec3 N = normalize(vertex_normal);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 normal_tangent = TBN * normalize(texture(normalTexture, texture_coord).xyz * 2.0 - vec3(1.0));
	out_normal = vec4(N, 1.0);
}
)";

const std::string lighting_vs_code = R"(
#version 450

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}

)";

const std::string lighting_fs_code = R"(
#version 450



//
// Geometry Uniform Buffers
// 
layout (binding = 0) uniform sampler2D samplerPosition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerSpecular;
layout (binding = 4) uniform sampler2D samplerDepth;
//layout (binding = 5) uniform sampler2D samplerShadowDepth;
//layout (binding = 5) uniform SunData 
//{
//	mat4 viewProj;
//} sun;

layout(binding = 5) uniform scene_ubo
{
    // ambient Strength, specular strength, specular shininess, empty
    vec4 ambientSpecular;
    vec4 cameraPosition;

	vec3 sunDirection;
	vec3 sunPosition;
} scene;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

//float ShadowCalculation(vec3 fragPos) 
//{
//	// TODO: Maybe this needs to be in a vertex shader?
//	vec4 fragPosLightSpace = sun.viewProj * vec4(fragPos, 1.0);
//
//    vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;
//
//	float shadowDepth = texture(samplerShadowDepth, projCoords.xy).r;
//	float currentDepth = projCoords.z;
//
//	// Check if current pixel is in shadow
//	//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
//	float shadow = currentDepth > shadowDepth ? 1.0 : 0.0;
//
//	return shadowDepth;	
//}

void main()
{
	// textures
	vec3 world_pos = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec3 albedo = texture(samplerAlbedo, inUV).rgb;
	float spec = texture(samplerSpecular, inUV).r;
	float depth = texture(samplerDepth, inUV).r;
	
	// TODO: Maybe this needs to be in a vertex shader?
	//vec4 fragPosLightSpace = sun.viewProj * vec4(world_pos, 1.0);
    //vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;
	//float shadowDepth = texture(samplerShadowDepth, projCoords.xy).r;

	vec3 lightColor = vec3(1.0);

	//////////////// AMBIENT ////////////////
	vec3 ambient = scene.ambientSpecular.rrr * lightColor;


	//////////////// DIFFUSE ////////////////
	vec3 light_dir = normalize(-scene.sunDirection);
	float diffuse_factor = max(dot(light_dir, normal), 0.0);
	vec3 diffuse = diffuse_factor * lightColor;

	//////////////// SPECULAR ////////////////
	vec3 camera_dir = normalize(scene.cameraPosition.xyz - world_pos);
	vec3 halfway_dir = normalize(camera_dir + light_dir);
	//vec3 light_reflect_dir = reflect(-scene.sunDirection, normal);

	float specFactor = max(dot(normal, halfway_dir), 0.0);
	vec3 specular = pow(specFactor, scene.ambientSpecular.b) * lightColor;

	//float shadow = ShadowCalculation(world_pos);

	//vec3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * albedo;
	vec3 result = (ambient + diffuse + specular) * albedo;
	outFragcolor = vec4(result, 1.0);
}
)";



const std::string skybox_vs_code = R"(
#version 450

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 texture_coord;

layout(binding = 0) uniform model_view_projection
{
    mat4 view;
    mat4 proj;
} mvp;

void main()
{
    texture_coord   = uv;

    gl_Position = mvp.proj * mat4(mat3(mvp.view)) * vec4(position, 1.0);
}
)";

const std::string skybox_fs_code = R"(
#version 450

layout(location = 0) in vec2 texture_coord;

layout(location = 0) out vec4 final_color;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main()
{
    final_color = vec4(texture(tex, texture_coord).rgb, 1.0);
}
)";







#endif