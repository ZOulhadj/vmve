#include "ui.hpp"

#include "rendering/vulkan/common.hpp"


#include "filesystem/vfs.hpp"
#include "filesystem/filesystem.hpp"

#include "logging.hpp"

static void custom_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 0.0f;
    style.TabRounding = 0.0f;
}

static void custom_colors() {
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.01f, 0.01f, 0.01f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.06f, 0.06f, 0.06f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.64f, 0.00f, 0.00f, 0.62f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.64f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

}


ImGuiContext* CreateUI(const VulkanRenderer* renderer, VkRenderPass renderPass)
{
    Logger::Info("Initializing user interface");

    ImGuiContext* context{};

    IMGUI_CHECKVERSION();
    context = ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDockingWithShift = true;
    io.IniFilename = nullptr;

    io.Fonts->AddFontDefault();

    custom_style();
    custom_colors();

    if (!ImGui_ImplGlfw_InitForVulkan(renderer->ctx.window->handle, true))
        return nullptr;

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance        = renderer->ctx.instance;
    init_info.PhysicalDevice  = renderer->ctx.device.gpu;
    init_info.Device          = renderer->ctx.device.device;
    init_info.QueueFamily     = renderer->ctx.device.graphics_index;
    init_info.Queue           = renderer->ctx.device.graphics_queue;
    init_info.PipelineCache   = nullptr;
    init_info.DescriptorPool  = renderer->descriptor_pool;
    init_info.Subpass         = 0;
    init_info.MinImageCount   = 2;
    init_info.ImageCount      = GetSwapchainImageCount();
    init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator       = nullptr;
    //init_info.CheckVkResultFn = VkCheck;

    if (!ImGui_ImplVulkan_Init(&init_info, renderPass))
        return nullptr;


    // Submit ImGui fonts to the GPU in order to be used during rendering.
    SubmitToGPU([&] { ImGui_ImplVulkan_CreateFontsTexture(renderer->submit.CmdBuffer); });

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    return context;
}

void DestroyUI(ImGuiContext* context)
{
    if (!context)
        return;


    Logger::Info("Terminating user interface");

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext(context);
}

void BeginUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();
}

void EndUI(std::vector<VkCommandBuffer>& buffers)
{
    ImGui::EndFrame();

    ImGui::Render();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();


    const uint32_t current_frame = GetFrameIndex();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffers[current_frame]);
}



void RecreateUITexture(std::vector<VkDescriptorSet>& texture_id, VkImageView view, VkSampler sampler, bool depth)
{
    for (std::size_t i = 0; i < texture_id.size(); ++i)
    {
        ImGui_ImplVulkan_RemoveTexture(texture_id[i]);
        texture_id[i] = ImGui_ImplVulkan_AddTexture(sampler, view, depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}