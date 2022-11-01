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
Shader CreateVertexShader(const std::string& code);
Shader CreateFragmentShader(const std::string& code);
void DestroyShader(Shader& shader);


#endif
