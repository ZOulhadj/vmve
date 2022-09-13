#ifndef MYENGINE_RENDERER_HPP
#define MYENGINE_RENDERER_HPP

enum class BufferMode
{
    Double  = 2,
    Triple = 3
};

enum class VSyncMode
{
    Disabled = VK_PRESENT_MODE_IMMEDIATE_KHR,
    Enabled  = VK_PRESENT_MODE_FIFO_KHR
};

struct VertexBuffer;

void CreateRenderer(BufferMode bufferMode, VSyncMode vsyncMode);
void DestroyRenderer();

VertexBuffer* CreateVertexBuffer(void* v, int vs, void* i, int is);
void BindVertexBuffer(const VertexBuffer* buffer);

void BeginFrame();
void EndFrame();

void RenderEntity(Entity* e);

#endif
