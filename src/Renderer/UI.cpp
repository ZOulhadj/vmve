#include "UI.hpp"

#include "Common.hpp"
#include "Renderer.hpp"


ImGuiContext* create_user_interface(VkRenderPass renderPass) {
    ImGuiContext* context = nullptr;

    const RendererContext* rc = GetRendererContext();

    IMGUI_CHECKVERSION();
    context = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.IniFilename = nullptr;
    /*if (!io.Fonts->AddFontFromFileTTF("assets/fonts/Karla-Regular.ttf", 16)) {
        printf("Failed to load required font for ImGui.\n");
        return nullptr;
    }*/

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForVulkan(rc->window->handle, true))
        return nullptr;

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance        = rc->instance;
    init_info.PhysicalDevice  = rc->device.gpu;
    init_info.Device          = rc->device.device;
    init_info.QueueFamily     = rc->device.graphics_index;
    init_info.Queue           = rc->device.graphics_queue;
    init_info.PipelineCache   = nullptr;
    init_info.DescriptorPool  = rc->pool;
    init_info.Subpass         = 0;
    init_info.MinImageCount   = 2;
    init_info.ImageCount      = 2;
    init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator       = nullptr;
    init_info.CheckVkResultFn = VkCheck;

    if (!ImGui_ImplVulkan_Init(&init_info, renderPass))
        return nullptr;


    // Submit ImGui fonts to the GPU in order to be used during rendering.
    SubmitToGPU([&] { ImGui_ImplVulkan_CreateFontsTexture(rc->submit.CmdBuffer); });

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    return context;
}

void destroy_user_interface(ImGuiContext* context) {
    if (!context)
        return;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext(context);
}

void render_ui() {
    ImGui::Render();
}
