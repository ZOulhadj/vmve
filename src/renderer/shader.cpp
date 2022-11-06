#include "shader.hpp"

#include "common.hpp"
#include "renderer.hpp"

static shaderc_shader_kind convert_shader_type(VkShaderStageFlagBits type)
{
    switch (type) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return shaderc_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return shaderc_fragment_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return shaderc_compute_shader;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return shaderc_geometry_shader;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return shaderc_tess_control_shader;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return shaderc_tess_evaluation_shader;
        case VK_SHADER_STAGE_ALL_GRAPHICS:
        case VK_SHADER_STAGE_ALL:
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
        case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
        case VK_SHADER_STAGE_MISS_BIT_KHR:
        case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
        case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
        case VK_SHADER_STAGE_TASK_BIT_NV:
        case VK_SHADER_STAGE_MESH_BIT_NV:
        case VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI:
            return shaderc_glsl_infer_from_source;
    }

    return shaderc_glsl_infer_from_source;
}


shader_compiler create_shader_compiler() {
    shader_compiler compiler{};

    // todo(zak): Check for potential initialization errors.
    compiler.compiler = shaderc_compiler_initialize();
    compiler.options  = shaderc_compile_options_initialize();

    shaderc_compile_options_set_optimization_level(compiler.options, shaderc_optimization_level_performance);

    return compiler;
}

void destroy_shader_compiler(shader_compiler& compiler) {
    shaderc_compile_options_release(compiler.options);
    shaderc_compiler_release(compiler.compiler);
}

shader create_shader(VkShaderStageFlagBits type, const std::string& code) {
    shader shader{};

    const renderer_context_t* rc = get_renderer_context();

    shaderc_compilation_result_t result;
    shaderc_compilation_status status;

    result = shaderc_compile_into_spv(rc->compiler.compiler,
                                      code.data(),
                                      code.size(),
                                      convert_shader_type(type),
                                      "",
                                      "main",
                                      rc->compiler.options);
    status = shaderc_result_get_compilation_status(result);

    if (status != shaderc_compilation_status_success) {
        printf("Failed to compile shader: %s\n", shaderc_result_get_error_message(result));
        return {};
    }

    // create shader module
    VkShaderModuleCreateInfo module_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    module_info.codeSize = u32(shaderc_result_get_length(result));
    module_info.pCode    = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));

    vk_check(vkCreateShaderModule(rc->device.device, &module_info, nullptr,
                                  &shader.handle));
    shader.type = type;

    return shader;
}


void destroy_shader(shader& shader) {
    const renderer_context_t* rc = get_renderer_context();

    vkDestroyShaderModule(rc->device.device, shader.handle, nullptr);
}

shader create_vertex_shader(const std::string& code) {
    return create_shader(VK_SHADER_STAGE_VERTEX_BIT, code);
}

shader create_fragment_shader(const std::string& code) {
    return create_shader(VK_SHADER_STAGE_FRAGMENT_BIT, code);
}

