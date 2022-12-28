#ifndef MY_ENGINE_SHADER_HPP
#define MY_ENGINE_SHADER_HPP

struct ShaderCompiler {
    shaderc_compiler_t compiler       = nullptr;
    shaderc_compile_options_t options = nullptr;
};

struct Shader {
    VkShaderModule handle      = nullptr;
    VkShaderStageFlagBits type;
};


ShaderCompiler CreateShaderCompiler();
void DestroyShaderCompiler(ShaderCompiler& compiler);
Shader CreateVertexShader(const std::string& code);
Shader CreateFragmentShader(const std::string& code);
void DestroyShader(Shader& shader);


#endif
