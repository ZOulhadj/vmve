#include "vertex_array.hpp"


#include "renderer.hpp"

Vertex_Array create_vertex_array(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    Vertex_Array vertexArray{};

    const Vulkan_Renderer* renderer = get_vulkan_renderer();

    const uint32_t vertices_size = vertices.size() * sizeof(Vertex);
    const uint32_t indices_size = indices.size() * sizeof(uint32_t);

    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Buffer vertexStagingBuffer = create_staging_buffer((void*)vertices.data(), vertices_size);
    Buffer indexStagingBuffer  = create_staging_buffer((void*)indices.data(), indices_size);

    vertexArray.vertex_buffer = create_gpu_buffer(vertices_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexArray.index_buffer  = create_gpu_buffer(indices_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vertexArray.index_count   = indices.size(); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    submit_to_gpu([&] {
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
    destroy_buffer(indexStagingBuffer);
    destroy_buffer(vertexStagingBuffer);

    return vertexArray;
}

void destroy_vertex_array(Vertex_Array& vertexArray) {
    destroy_buffer(vertexArray.index_buffer);
    destroy_buffer(vertexArray.vertex_buffer);
}

void bind_vertex_array(const std::vector<VkCommandBuffer>& buffers, const Vertex_Array& vertexArray) {
    uint32_t current_frame = get_frame_index();

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(buffers[current_frame], 0, 1, &vertexArray.vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(buffers[current_frame], vertexArray.index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

