#include <engine.h>

#include "vmve.hpp"
#include "security.hpp"
#include "ui.hpp"

void KeyCallback(Engine* engine, int keycode)
{
    // NOTE: 256 == ESCAPE

    if (keycode == 256)
    {
        viewportActive = !viewportActive;

        EngineSetCursorMode(engine, viewportActive);
    }
}

int main()
{
    // TODO: Icon needs to be bundled with the executable and this needs to be
    // the case for both the windows icon and glfw window icon
    EngineInfo info{};
    info.iconPath = "icon.png";
    info.windowWidth = 1280;
    info.windowHeight = 720;

    Engine* engine = EngineInitialize(info);

    RegisterKeyCallback(engine, KeyCallback);


    //EngineSetEnviromentMap("environment_map.jpg");
    EngineCreateCamera(engine, 45.0f, 20.0f);

    EngineEnableUIPass(engine);


//    EngineAddModel(engine, "C:\\Users\\zakar\\Projects\\vmve\\vmve\\assets\\models\\sponza\\sponza.obj", true);
//    EngineAddInstance(engine, 0, 0.0f, 0.0f, 0.0f);


    while (EngineUpdate(engine))
    {
        // Updating
        if (viewportActive)
        {
            EngineUpdateInput(engine);
            EngineUpdateCameraView(engine);
        }

        if (resize_viewport)
        {
            EngineUpdateCameraProjection(engine, viewport_width, viewport_height);
        }


        // Begin rendering commands
        EngineBeginRender(engine);

        // Deferred Rendering
        EngineRender(engine);

        // UI Rendering
        EngineBeginUIPass();
        BeginDocking();
        RenderMainMenu(engine);
        RenderDockspace();
        RenderObjectWindow(engine);
        RenderGlobalWindow(engine);
        RenderConsoleWindow(engine);
        RenderViewportWindow();
        EndDocking();
        EngineEndUIPass();

        // Display to screen
        EnginePresent(engine);
    }

    EngineTerminate(engine);

    return 0;
}





// The viewport must resize before rendering to texture. If it were
// to resize within the UI functions then we would be attempting to
// recreate resources using a new size before submitting to the GPU
// which causes an error. Need to figure out how to do this properly.
#if 0
if (resize_viewport)
{
    VkExtent2D size{};
    size.width = viewport_size.x;
    size.height = viewport_size.y;

    UpdateProjection(engine->camera, size.width, size.height);

    WaitForGPU();

    // TODO: You would think that resizing is easy.... Yet, there seems to
    // be multiple issues such as stuttering, and image layout being 
    // undefined. Needs to be fixed.
#if 0
    ResizeFramebuffer(offscreenPass, size);
    ResizeFramebuffer(compositePass, size);

    // TODO: update all image buffer descriptor sets
    positions = AttachmentsToImages(offscreenPass.attachments, 0);
    normals = AttachmentsToImages(offscreenPass.attachments, 1);
    colors = AttachmentsToImages(offscreenPass.attachments, 2);
    speculars = AttachmentsToImages(offscreenPass.attachments, 3);
    depths = AttachmentsToImages(offscreenPass.attachments, 4);

    viewport = AttachmentsToImages(compositePass.attachments, 0);

    std::vector<VkDescriptorSetLayoutBinding> compositeBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
    };
    UpdateBinding(compositeSets, compositeBindings[0], positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[1], normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[2], colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[3], speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[4], depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, gFramebufferSampler);

    // Create descriptor sets for g-buffer images for UI


    // TODO: Causes stuttering for some reason
    // This is related to the order, creating the textures the other way seemed
    // to fix it...
    for (auto& image : viewport)
        RecreateUITexture(viewportUI, image.view, gFramebufferSampler);

    for (std::size_t i = 0; i < GetSwapchainImageCount(); ++i)
    {
        RecreateUITexture(positionsUI, positions[i].view, gFramebufferSampler);
        RecreateUITexture(normalsUI, normals[i].view, gFramebufferSampler);
        RecreateUITexture(colorsUI, colors[i].view, gFramebufferSampler);
        RecreateUITexture(specularsUI, speculars[i].view, gFramebufferSampler);
        RecreateUITexture(depthsUI, depths[i].view, gFramebufferSampler, true);
    }
#endif

    resize_viewport = false;
}
#endif
