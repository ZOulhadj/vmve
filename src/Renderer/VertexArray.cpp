#include "VertexArray.hpp"


#include "Renderer.hpp"

EntityModel* CreateVertexArray(void* v, int vs, void* i, int is) {
    auto r = new EntityModel();


    const RendererContext* rc = GetRendererContext();


    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Buffer vertexStagingBuffer = CreateStagingBuffer(v, vs);
    Buffer indexStagingBuffer  = CreateStagingBuffer(i, is);

    r->vertex_buffer = CreateGPUBuffer(vs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    r->index_buffer  = CreateGPUBuffer(is, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    r->index_count   = is / sizeof(uint32_t); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    SubmitToGPU([&] {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vs;
        index_copy_info.size = is;

        vkCmdCopyBuffer(rc->submit.CmdBuffer, vertexStagingBuffer.buffer, r->vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(rc->submit.CmdBuffer, indexStagingBuffer.buffer, r->index_buffer.buffer, 1,
                        &index_copy_info);
    });


    // We can now free the staging buffer memory as it is no longer required
    DestroyBuffer(indexStagingBuffer);
    DestroyBuffer(vertexStagingBuffer);

    return r;
}

void DestroyVertexArray(EntityModel* model) {
    DestroyBuffer(model->index_buffer);
    DestroyBuffer(model->vertex_buffer);

    delete model;
}

void BindVertexArray(const EntityModel* model) {
    const VkCommandBuffer& cmd_buffer = GetCommandBuffer();

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &model->vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buffer, model->index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

