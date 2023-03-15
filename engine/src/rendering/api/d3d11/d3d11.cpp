#include "pch.h"
#include <d3d11.h>

#include "rendering/renderer.h"

#include "logging.h"

struct engine_renderer
{
    ID3D11Device* device;
    ID3D11DeviceContext* device_context;

    IDXGISwapChain* swapchain;
    ID3D11RenderTargetView* render_target_view;
};


engine_renderer* initialize_renderer(engine_window* window)
{
    engine_renderer* renderer = new engine_renderer();

    DXGI_SWAP_CHAIN_DESC swapchain_info{};
    swapchain_info.BufferDesc.Width = 1280;
    swapchain_info.BufferDesc.Height = 720;
    swapchain_info.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_info.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_info.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    swapchain_info.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_info.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_info.SampleDesc.Count = 1;
    swapchain_info.SampleDesc.Quality = 0;
    swapchain_info.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_info.BufferCount = 1;
    swapchain_info.OutputWindow = glfwGetWin32Window(window->handle);
    swapchain_info.Windowed = false;
    swapchain_info.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT result = S_OK;
    result = D3D11CreateDeviceAndSwapChain(
        nullptr, 
        D3D_DRIVER_TYPE_HARDWARE, 
        nullptr, 
        NULL, 
        nullptr,
        NULL, 
        D3D11_SDK_VERSION,
        &swapchain_info,
        &renderer->swapchain,
        &renderer->device,
        nullptr,
        &renderer->device_context);

    if (FAILED(result)) {
        print_error("Failed to initialize device and swapchain\n");
        return nullptr;
    }


    ID3D11Texture2D* back_buffer;
    result = renderer->swapchain->GetBuffer(
        0, 
        __uuidof(ID3D11Texture2D), 
        reinterpret_cast<void**>(&back_buffer));
    if (FAILED(result)) {
        print_error("Failed to get swapchain backbuffer.\n");
        return nullptr;
    }

    result = renderer->device->CreateRenderTargetView(back_buffer, nullptr, &renderer->render_target_view);
    if (FAILED(result)) {
        print_error("Failed to create back buffer render target view.");
        return nullptr;
    }

    renderer->device_context->OMSetRenderTargets(1, &renderer->render_target_view, nullptr);


    return nullptr;
}

void terminate_renderer(engine_renderer* renderer)
{
    if (!renderer)
        return;

    renderer->render_target_view->Release();
    renderer->swapchain->Release();
    renderer->device_context->Release();
    renderer->device->Release();

    delete renderer;
}


