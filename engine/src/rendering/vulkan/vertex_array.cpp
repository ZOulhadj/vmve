#include "vertex_array.hpp"


#include "renderer.hpp"

VertexArray CreateVertexArray(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    VertexArray vertexArray{};

    const VulkanRenderer* renderer = GetRenderer();

    const uint32_t vertices_size = vertices.size() * sizeof(Vertex);
    const uint32_t indices_size = indices.size() * sizeof(uint32_t);

    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Buffer vertexStagingBuffer = CreateStagingBuffer((void*)vertices.data(), vertices_size);
    Buffer indexStagingBuffer  = CreateStagingBuffer((void*)indices.data(), indices_size);

    vertexArray.vertex_buffer = CreateGPUBuffer(vertices_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexArray.index_buffer  = CreateGPUBuffer(indices_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vertexArray.index_count   = indices.size(); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    SubmitToGPU([&] {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vertices_size;
        index_copy_info.size  = indices_size;

        vkCmdCopyBuffer(renderer->submit.CmdBuffer, vertexStagingBuffer.buffer,
                        vertexArray.vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(renderer->submit.CmdBuffer, indexStagingBuffer.buffer,
                        vertexArray.index_buffer.buffer, 1,
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

void BindVertexArray(const std::vector<VkCommandBuffer>& buffers, const VertexArray& vertexArray) {

    uint32_t current_frame = GetFrameIndex();

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(buffers[current_frame], 0, 1, &vertexArray.vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(buffers[current_frame], vertexArray.index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

