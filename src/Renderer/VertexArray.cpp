#include "VertexArray.hpp"


#include "Renderer.hpp"

VertexArray CreateVertexArray(void* v, int vs, void* i, int is) {
    VertexArray vertexArray{};

    const RendererContext* rc = GetRendererContext();


    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Buffer vertexStagingBuffer = CreateStagingBuffer(v, vs);
    Buffer indexStagingBuffer  = CreateStagingBuffer(i, is);

    vertexArray.vertex_buffer = CreateGPUBuffer(vs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexArray.index_buffer  = CreateGPUBuffer(is, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vertexArray.index_count   = is / sizeof(uint32_t); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    SubmitToGPU([&] {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vs;
        index_copy_info.size = is;

        vkCmdCopyBuffer(rc->submit.CmdBuffer, vertexStagingBuffer.buffer, vertexArray.vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(rc->submit.CmdBuffer, indexStagingBuffer.buffer, vertexArray.index_buffer.buffer, 1,
                        &index_copy_info);
    });


    // We can now free the staging buffer memory as it is no longer required
    DestroyBuffer(indexStagingBuffer);
    DestroyBuffer(vertexStagingBuffer);

    return vertexArray;
}

void DestroyVertexArray(VertexArray& vertexArray) {
    DestroyBuffer(vertexArray.index_buffer);
    DestroyBuffer(vertexArray.vertex_buffer);
}

void BindVertexArray(const VertexArray& vertexArray) {
    const VkCommandBuffer& cmd_buffer = GetCommandBuffer();

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vertexArray.vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buffer, vertexArray.index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

