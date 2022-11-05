#ifndef MY_ENGINE_SHADER_HPP
#define MY_ENGINE_SHADER_HPP

struct ShaderCompiler {
    shaderc_compiler_t compiler;
    shaderc_compile_options_t options;
};

struct Shader {
    VkShaderModule handle;
    VkShaderStageFlagBits type;
};


ShaderCompiler CreateShaderCompiler();
void DestroyShaderCompiler(ShaderCompiler& compiler);
Shader create_vertex_shader(const std::string& code);
Shader create_fragment_shader(const std::string& code);
void destroy_shader(Shader& shader);


#endif
