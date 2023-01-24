#include <engine.h>

#include "vmve.hpp"
#include "security.hpp"
#include "ui.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

static void key_callback(Engine* engine, int keycode);

bool notFullScreen = true;


int main() {
    // TODO: Icon needs to be bundled with the executable and this needs to be
    // the case for both the windows icon and glfw window icon
    Engine_Info info{};
    info.appName = "VMVE";
    info.windowWidth = 1280;
    info.windowHeight = 720;

    Engine* engine = engine_initialize(info);

    engine_register_key_callback(engine, key_callback);
    
    engine_create_camera(engine, 45.0f, 20.0f);

    engine_enable_ui(engine);

    engine_add_model(engine, "C:\\Users\\zakar\\Projects\\vmve\\vmve\\assets\\models\\backpack\\backpack.obj", false);
    engine_add_instance(engine, 0, 0.0f, 0.0f, 0.0f);

    
    while (engine_update(engine)) {
        // Updating
        if (viewportActive) {
            engine_update_input(engine);
            engine_update_camera_view(engine);
        }

        if (resizeViewport) {
            engine_update_camera_projection(engine, viewport_width, viewport_height);
        }


        if (engine_begin_render(engine)) {
            // Deferred Rendering
            engine_render(engine);

            // UI Rendering
            engine_begin_ui_pass();
            render_ui(engine, !notFullScreen);
            engine_end_ui_pass();
       
            engine_present(engine);
        }
        
    }

    engine_terminate(engine);

    return 0;
}


void key_callback(Engine* engine, int keycode)
{
#define KEY_F1 290
#define KEY_TAB 258
#define KEY_E 69
#define KEY_T 84
#define KEY_R 82
#define KEY_S 83

    // NOTE: 290 == F1
    // NOTE: 258 == TAB
    // NOTE: 69 == E
    if (keycode == KEY_F1)
    {
        viewportActive = !viewportActive;

        engine_set_cursor_mode(engine, viewportActive);
    }


    if (keycode == KEY_TAB) {
        notFullScreen = !notFullScreen;

        if (notFullScreen) {
            firstTimeNormal = true;
            firstTimeFullScreen = false;
            viewportActive = false;
            engine_set_cursor_mode(engine, 0);
        } else {
            firstTimeFullScreen = true;
            firstTimeNormal = false;
            viewportActive = true;
            engine_set_cursor_mode(engine, 1);
        }
    }

    if (keycode == KEY_E) {
        object_edit_mode = !object_edit_mode;
    }

    if (keycode == KEY_T) {
        guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
    }

    if (keycode == KEY_R) {
        guizmo_operation = ImGuizmo::OPERATION::ROTATE;
    }
    if (keycode == KEY_S) {
        guizmo_operation = ImGuizmo::OPERATION::SCALE;
    }


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
