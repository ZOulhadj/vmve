#ifndef MY_ENGINE_SHADER_HPP
#define MY_ENGINE_SHADER_HPP

struct Shader_Compiler {
    shaderc_compiler_t compiler       = nullptr;
    shaderc_compile_options_t options = nullptr;
};

struct Shader {
    VkShaderModule handle      = nullptr;
    VkShaderStageFlagBits type;
};


Shader_Compiler create_shader_compiler();
void destroy_shader_compiler(Shader_Compiler& compiler);
Shader create_vertex_shader(const std::string& code);
Shader create_fragment_shader(const std::string& code);
void destroy_shader(Shader& shader);


#endif
