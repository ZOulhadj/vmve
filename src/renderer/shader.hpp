#ifndef MY_ENGINE_SHADER_HPP
#define MY_ENGINE_SHADER_HPP

struct shader_compiler {
    shaderc_compiler_t compiler       = nullptr;
    shaderc_compile_options_t options = nullptr;
};

struct shader {
    VkShaderModule handle      = nullptr;
    VkShaderStageFlagBits type;
};


shader_compiler create_shader_compiler();
void destroy_shader_compiler(shader_compiler& compiler);
shader create_vertex_shader(const std::string& code);
shader create_fragment_shader(const std::string& code);
void destroy_shader(shader& shader);


#endif
