#ifndef MY_ENGINE_VULKAN_SHADER_H
#define MY_ENGINE_VULKAN_SHADER_H

struct shader_compiler
{
    shaderc_compiler_t compiler       = nullptr;
    shaderc_compile_options_t options = nullptr;
};

struct vk_shader
{
    VkShaderModule handle      = nullptr;
    VkShaderStageFlagBits type;
};


shader_compiler create_shader_compiler();
void destroy_shader_compiler(shader_compiler& compiler);
vk_shader create_vertex_shader(const std::string& code);
vk_shader create_pixel_shader(const std::string& code);
void destroy_shader(vk_shader& shader);


#endif
