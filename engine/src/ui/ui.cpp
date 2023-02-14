#include "ui.h"

#include "rendering/api/vulkan/common.h"


#include "filesystem/vfs.h"
#include "filesystem/filesystem.h"

#include "logging.h"

ImGuiContext* create_ui(const vk_renderer* renderer, VkRenderPass renderPass)
{
    print_log("Initializing user interface\n");

    ImGuiContext* context{};

    IMGUI_CHECKVERSION();
    context = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDockingWithShift = true;
    io.IniFilename = nullptr;

    if (!ImGui_ImplGlfw_InitForVulkan(renderer->ctx.window->handle, true))
        return nullptr;

    // Set function pointers for when dynamically linking to vulkan
    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void*) { 
        return vkGetInstanceProcAddr(volkGetLoadedInstance(), function_name);
    });

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance        = renderer->ctx.instance;
    init_info.PhysicalDevice  = renderer->ctx.device->gpu;
    init_info.Device          = renderer->ctx.device->device;
    init_info.QueueFamily     = renderer->ctx.device->graphics_index;
    init_info.Queue           = renderer->ctx.device->graphics_queue;
    init_info.PipelineCache   = nullptr;
    init_info.DescriptorPool  = renderer->descriptor_pool;
    init_info.Subpass         = 0;
    init_info.MinImageCount   = 2;
    init_info.ImageCount      = get_swapchain_image_count();
    init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator       = nullptr;
    //init_info.CheckVkResultFn = VkCheck;

    if (!ImGui_ImplVulkan_Init(&init_info, renderPass))
        return nullptr;

    return context;
}

void create_font_textures()
{
    // Submit ImGui fonts to the GPU in order to be used during rendering.
    submit_to_gpu([&](VkCommandBuffer cmd_buffer) { 
        ImGui_ImplVulkan_CreateFontsTexture(cmd_buffer); 
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void destroy_ui(ImGuiContext* context)
{
    if (!context)
        return;

    print_log("Terminating user interface\n");

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext(context);
}

void begin_ui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();
}

void end_ui(std::vector<VkCommandBuffer>& buffers)
{
    ImGui::EndFrame();

    ImGui::Render();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();


    const uint32_t current_frame = get_frame_index();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffers[current_frame]);
}



void recreate_ui_texture(std::vector<VkDescriptorSet>& texture_id, VkImageView view, VkSampler sampler, bool depth)
{
    for (std::size_t i = 0; i < texture_id.size(); ++i) {
        ImGui_ImplVulkan_RemoveTexture(texture_id[i]);
        texture_id[i] = ImGui_ImplVulkan_AddTexture(sampler, view, depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}