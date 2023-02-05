#include <engine.h>

#include "vmve.hpp"
#include "security.hpp"
#include "ui.hpp"
#include "misc.hpp"

#include <imgui.h>
#include <ImGuizmo.h>


static void key_callback(Engine* engine, int keycode);
static void resize_callback(Engine* engine, int width, int height);

bool notFullScreen = true;

int main()
{
    Engine* engine = nullptr;

    bool engine_initialized = engine_initialize(engine, "VMVE", 1280, 720);
    if (!engine_initialized) {
        engine_export_logs_to_file(engine, "vmve_crash_log.txt");
        engine_terminate(engine);

        return -1;
    }

    // Set application icon
    int icon_width, icon_height;
    unsigned char* app_icon = get_app_icon(&icon_width, &icon_height);
    engine_set_window_icon(engine, app_icon, icon_width, icon_height);

    // Register application event callbacks with the engine.
    engine_callbacks callbacks;
    callbacks.key_callback = key_callback;
    callbacks.resize_callback = resize_callback;
    engine_set_callbacks(engine, callbacks);

    // Configure engine properties
    engine_enable_ui(engine);
    engine_create_camera(engine, 60.0f, 50.0f);


    while (engine_update(engine)) {
        //  Only update the camera view if the viewport is currently in focus.
        if (viewport_active) {
            engine_update_input(engine);
            engine_update_camera_view(engine);
        }

        // If the main viewport is resized then we should update the camera 
        // projection to take into account the new dimensions.
        if (should_resize_viewport) {
            engine_update_camera_projection(engine, viewport_width, viewport_height);

            should_resize_viewport = false;
        }



        if (engine_begin_render(engine)) {
            // Main render pass that renders the scene geometry.
            engine_render(engine);

            // Render all UI elements 
            engine_begin_ui_pass();
            render_ui(engine, !notFullScreen);
            engine_end_ui_pass();
       
            // Request the GPU to execute all rendering operations and display
            // final result onto the screen.
            engine_present(engine);
        }
        
    }

    // Terminate the application and free all resources.
    engine_terminate(engine);

    return 0;
}


void key_callback(Engine* engine, int keycode)
{
#define KEY_F1 290
#define KEY_F2 291
#define KEY_F3 292
#define KEY_F4 293
#define KEY_F5 294
#define KEY_F6 295
#define KEY_F7 296
#define KEY_F8 297
#define KEY_F9 298
#define KEY_F10 299
#define KEY_F11 300
#define KEY_F12 301
#define KEY_TAB 258
#define KEY_E 69
#define KEY_T 84
#define KEY_R 82
#define KEY_S 83

    if (keycode == KEY_F1) {
        viewport_active = !viewport_active;

        engine_set_cursor_mode(engine, viewport_active);
    }


    if (keycode == KEY_F2) {
        notFullScreen = !notFullScreen;

#if 0
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
#endif
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

void resize_callback(Engine* engine, int width, int height)
{
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
