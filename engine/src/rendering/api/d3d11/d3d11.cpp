#include "pch.h"
#include <d3d11.h>
#include <d3dcompiler.h>

#include "rendering/renderer.h"

#include "logging.h"

#include "rendering/vertex.h"

// IA
// RS
// PS
// OM

namespace engine {
    struct Engine_Renderer
    {
        const engine_window* window;

        ID3D11Device* device;
        ID3D11DeviceContext* context;

        IDXGISwapChain* swapchain;
        ID3D11RenderTargetView* render_target_view;
        ID3D11DepthStencilView* depth_stencil_view;
        ID3D11Texture2D* depth_stencil_buffer;


        // scene related information
        ID3D10Blob* vertex_shader_buffer;
        ID3D10Blob* pixel_shader_buffer;
        ID3D11VertexShader* vertex_shader;
        ID3D11PixelShader* pixel_shader;

        ID3D11InputLayout* vertex_input_layout;
        ID3D11Buffer* vertex_buffer;
        ID3D11Buffer* index_buffer;

        ID3D11Buffer* uniform_buffer;

    };


    Engine_Renderer* initialize_renderer(const engine_window* window)
    {
        Engine_Renderer* renderer = new Engine_Renderer();

        renderer->window = window;

        DXGI_SWAP_CHAIN_DESC swapchain_info{};
        swapchain_info.BufferDesc.Width = renderer->window->width;
        swapchain_info.BufferDesc.Height = renderer->window->height;
        swapchain_info.BufferDesc.RefreshRate.Numerator = 60;
        swapchain_info.BufferDesc.RefreshRate.Denominator = 1;
        swapchain_info.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_info.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapchain_info.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapchain_info.SampleDesc.Count = 1;
        swapchain_info.SampleDesc.Quality = 0;
        swapchain_info.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_info.BufferCount = 1;
        swapchain_info.OutputWindow = glfwGetWin32Window(window->handle);
        swapchain_info.Windowed = !window->fullscreen;
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
            &renderer->context);

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

        // Once the render target has been created we no longer require a pointer to the back buffer
        back_buffer->Release();


        // Setup depth stencil
        D3D11_TEXTURE2D_DESC depthStencilDesc;
        depthStencilDesc.Width = renderer->window->width;
        depthStencilDesc.Height = renderer->window->height;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilDesc.CPUAccessFlags = 0;
        depthStencilDesc.MiscFlags = 0;

        //Create the Depth/Stencil View
        result = renderer->device->CreateTexture2D(&depthStencilDesc, NULL, &renderer->depth_stencil_buffer);
        if (FAILED(result)) {
            print_error("Failed to create depth stencil buffer\n");
            return nullptr;
        }

        result = renderer->device->CreateDepthStencilView(renderer->depth_stencil_buffer, nullptr, &renderer->depth_stencil_view);
        if (FAILED(result)) {
            print_error("Failed to create depth stencil view\n");
            return nullptr;
        }


        D3D11_RASTERIZER_DESC raster_desc{};
        raster_desc.FillMode = D3D11_FILL_SOLID;
        raster_desc.CullMode = D3D11_CULL_NONE;
        raster_desc.FrontCounterClockwise = true;
        raster_desc.DepthBias = 0;
        raster_desc.DepthBiasClamp = 0.0f;
        raster_desc.SlopeScaledDepthBias = 0.0f;
        raster_desc.DepthClipEnable = true;
        raster_desc.ScissorEnable = false;
        raster_desc.MultisampleEnable = false;
        raster_desc.AntialiasedLineEnable = false;

        ID3D11RasterizerState* rasterizer_state;
        renderer->device->CreateRasterizerState(&raster_desc, &rasterizer_state);
        renderer->context->RSSetState(rasterizer_state);
        rasterizer_state->Release();

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = renderer->window->width;
        viewport.Height = renderer->window->height;
        renderer->context->RSSetViewports(1, &viewport);


        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc{};
        // Depth test parameters
        depth_stencil_desc.DepthEnable = true;
        depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth_stencil_desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
#if 0
        // Stencil test parameters
        depth_stencil_desc.StencilEnable = false;
        depth_stencil_desc.StencilReadMask = 0xFF;
        depth_stencil_desc.StencilWriteMask = 0xFF;

        // Stencil operations if pixel is front-facing
        depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        // Stencil operations if pixel is back-facing
        depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
#endif

        ID3D11DepthStencilState* depth_stencil_state;
        renderer->device->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state);
        renderer->context->OMSetDepthStencilState(depth_stencil_state, 1);
        depth_stencil_state->Release();


        renderer->context->OMSetRenderTargets(1, &renderer->render_target_view, renderer->depth_stencil_view);

        // IA
        // RS
        // OM

        return renderer;
    }

    void terminate_renderer(Engine_Renderer* renderer)
    {
        if (!renderer)
            return;


        renderer->depth_stencil_view->Release();
        renderer->depth_stencil_buffer->Release();
        renderer->render_target_view->Release();
        renderer->swapchain->Release();
        renderer->context->Release();
        renderer->device->Release();

        delete renderer;
    }

    template <typename T>
    void create_vertex_buffer(Engine_Renderer* renderer, const std::vector<T>& vertices)
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.ByteWidth = sizeof(T) * vertices.size();
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA sub_desc{};
        sub_desc.pSysMem = vertices.data();
        HRESULT hr = renderer->device->CreateBuffer(&buffer_desc, &sub_desc, &renderer->vertex_buffer);
        if (FAILED(hr)) {
            print_error("Failed to create vertex buffer\n");
            return;
        }

        UINT stride = sizeof(T);
        UINT offset = 0;
        renderer->context->IASetVertexBuffers(0, 1, &renderer->vertex_buffer, &stride, &offset);
    }

    void create_index_buffer(Engine_Renderer* renderer, const std::vector<uint32_t>& indices)
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.ByteWidth = sizeof(uint32_t) * indices.size();
        buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA sub_desc{};
        sub_desc.pSysMem = indices.data();
        HRESULT hr = renderer->device->CreateBuffer(&buffer_desc, &sub_desc, &renderer->index_buffer);
        if (FAILED(hr)) {
            print_error("Failed to create index buffer\n");
            return;
        }

        renderer->context->IASetIndexBuffer(renderer->index_buffer, DXGI_FORMAT_R32_UINT, 0);
    }

    void create_uniform_buffer(Engine_Renderer* renderer, std::size_t size)
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.ByteWidth = static_cast<UINT>(size);
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = 0;

        HRESULT hr = renderer->device->CreateBuffer(&buffer_desc, nullptr, &renderer->uniform_buffer);
        if (FAILED(hr)) {
            print_error("Failed to create uniform buffer\n");
            return;
        }
    }

    void update_uniform_buffer(Engine_Renderer* renderer, void* buffer)
    {
        renderer->context->UpdateSubresource(renderer->uniform_buffer, 0, nullptr, buffer, 0, 0);
        renderer->context->VSSetConstantBuffers(0, 1, &renderer->uniform_buffer);
    }


    bool initialize_renderer_scene(Engine_Renderer* renderer)
    {
        HRESULT hr = S_OK;

        //////////////////////////////////////////////////////////////////////////

        const wchar_t shader_file[] = L"../engine/src/rendering/shaders/d3d11/default.fx";

        //Compile Shaders from shader file
        ID3DBlob* error_blob;
        hr = D3DCompileFromFile(
            shader_file,
            nullptr,
            nullptr,
            "main_vertex",
            "vs_4_0",
            0,
            0,
            &renderer->vertex_shader_buffer,
            &error_blob
        );

        if (FAILED(hr)) {
            std::string error(static_cast<char*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize());
            print_error("%s\n", error.c_str());
            return false;
        }

        hr = D3DCompileFromFile(
            shader_file,
            nullptr,
            nullptr,
            "main_pixel",
            "ps_4_0",
            0,
            0,
            &renderer->pixel_shader_buffer,
            &error_blob
        );

        if (FAILED(hr)) {
            std::string error(static_cast<char*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize());
            print_error("%s\n", error.c_str());
            return false;
        }

        //Create the Shader Objects
        hr = renderer->device->CreateVertexShader(renderer->vertex_shader_buffer->GetBufferPointer(),
            renderer->vertex_shader_buffer->GetBufferSize(), NULL, &renderer->vertex_shader);
        hr = renderer->device->CreatePixelShader(renderer->pixel_shader_buffer->GetBufferPointer(),
            renderer->pixel_shader_buffer->GetBufferSize(), NULL, &renderer->pixel_shader);

        //Set Vertex and Pixel Shaders
        renderer->context->VSSetShader(renderer->vertex_shader, nullptr, 0);
        renderer->context->PSSetShader(renderer->pixel_shader, nullptr, 0);


        //////////////////////////////////////////////////////////////////////////


        //Create the vertex buffer

        struct v {
            float x; float y; float z;
            float r; float g; float b; float a;
        };
        std::vector<v> vertices = {
            { -0.5f, -0.5f, 0.5f,   1.0f, 0.0f, 0.0f, 1.0f },
            { -0.5f,  0.5f, 0.5f,   0.0f, 1.0f, 0.0f, 1.0f },
            { 0.5f,  0.5f, 0.5f,   0.0f, 1.0f, 1.0f, 1.0f},
            { 0.5f, -0.5f, 0.5f,   0.0f, 0.0f, 1.0f, 1.0f}
        };

        const std::vector<uint32_t> indices{
            0, 1, 2,
            0, 2, 3
        };


        const std::array<D3D11_INPUT_ELEMENT_DESC, 2> vertex_layout{
            D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            D3D11_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        create_vertex_buffer(renderer, vertices);
        create_index_buffer(renderer, indices);


        //Create the Input Layout
        renderer->device->CreateInputLayout(vertex_layout.data(), vertex_layout.size(), renderer->vertex_shader_buffer->GetBufferPointer(),
            renderer->vertex_shader_buffer->GetBufferSize(), &renderer->vertex_input_layout);

        //Set the Input Layout
        renderer->context->IASetInputLayout(renderer->vertex_input_layout);

        //Set Primitive Topology
        renderer->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //////////////////////////////////////////////////////////////////////////


        return true;
    }



    void terminate_renderer_scene(Engine_Renderer* renderer)
    {
        renderer->uniform_buffer->Release();

        renderer->vertex_input_layout->Release();
        renderer->vertex_buffer->Release();
        renderer->pixel_shader->Release();
        renderer->vertex_shader->Release();
        renderer->pixel_shader_buffer->Release();
        renderer->vertex_shader_buffer->Release();
    }


    void renderer_clear(Engine_Renderer* renderer, const std::array<float, 4>& clear_color)
    {
        renderer->context->ClearRenderTargetView(renderer->render_target_view, clear_color.data());
        renderer->context->ClearDepthStencilView(renderer->depth_stencil_view, D3D11_CLEAR_DEPTH, 0.0f, 0);

    }

    void renderer_draw(Engine_Renderer* renderer)
    {
        renderer->context->DrawIndexed(6, 0, 0);
    }

    void renderer_present(Engine_Renderer* renderer)
    {
        renderer->swapchain->Present(1, 0);
    }
}