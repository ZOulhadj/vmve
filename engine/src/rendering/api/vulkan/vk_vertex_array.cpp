#include "pch.h"
#include "vk_vertex_array.h"


#include "vk_renderer.h"

vk_vertex_array create_vertex_array(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices)
{
    vk_vertex_array vertexArray{};

    const VkDeviceSize vertices_size = vertices.size() * sizeof(vertex);
    const uint32_t indices_size = static_cast<uint32_t>(indices.size()) * sizeof(uint32_t);

    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Vk_Buffer vertexStagingBuffer = create_staging_buffer((void*)vertices.data(), vertices_size);
    Vk_Buffer indexStagingBuffer  = create_staging_buffer((void*)indices.data(), indices_size);

    vertexArray.vertex_buffer = create_gpu_buffer(vertices_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexArray.index_buffer  = create_gpu_buffer(indices_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vertexArray.index_count   = static_cast<uint32_t>(indices.size());

    // Upload data to the GPU
    submit_to_gpu([&](VkCommandBuffer cmd_buffer) {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vertices_size;
        index_copy_info.size  = static_cast<VkDeviceSize>(indices_size);

        vkCmdCopyBuffer(cmd_buffer, vertexStagingBuffer.buffer,
                        vertexArray.vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(cmd_buffer, indexStagingBuffer.buffer,
                        vertexArray.index_buffer.buffer, 1,
                        &index_copy_info);
    });


    // We can now free the staging buffer memory as it is no longer required
    destroy_buffer(indexStagingBuffer);
    destroy_buffer(vertexStagingBuffer);

    return vertexArray;
}

void destroy_vertex_array(vk_vertex_array& vertexArray) {
    destroy_buffer(vertexArray.index_buffer);
    destroy_buffer(vertexArray.vertex_buffer);
}

void bind_vertex_array(const std::vector<VkCommandBuffer>& buffers, const vk_vertex_array& vertexArray) {
    uint32_t current_frame = get_frame_buffer_index();

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(buffers[current_frame], 0, 1, &vertexArray.vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(buffers[current_frame], vertexArray.index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

