#include "vertex_array.hpp"


#include "renderer.hpp"

vertex_array_t create_vertex_array(void* v, int vs, void* i, int is) {
    vertex_array_t vertexArray{};

    const renderer_context_t* rc = get_renderer_context();


    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    buffer_t vertexStagingBuffer = create_staging_buffer(v, vs);
    buffer_t indexStagingBuffer  = create_staging_buffer(i, is);

    vertexArray.vertex_buffer = create_gpu_buffer(vs,
                                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexArray.index_buffer  = create_gpu_buffer(is,
                                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vertexArray.index_count   = is / sizeof(uint32_t); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    submit_to_gpu([&] {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vs;
        index_copy_info.size = is;

        vkCmdCopyBuffer(rc->submit.CmdBuffer, vertexStagingBuffer.buffer,
                        vertexArray.vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(rc->submit.CmdBuffer, indexStagingBuffer.buffer,
                        vertexArray.index_buffer.buffer, 1,
                        &index_copy_info);
    });


    // We can now free the staging buffer memory as it is no longer required
    destroy_buffer(indexStagingBuffer);
    destroy_buffer(vertexStagingBuffer);

    return vertexArray;
}

void destroy_vertex_array(vertex_array_t& vertexArray) {
    destroy_buffer(vertexArray.index_buffer);
    destroy_buffer(vertexArray.vertex_buffer);
}

void bind_vertex_array(const vertex_array_t& vertexArray) {
    const VkCommandBuffer& cmd_buffer = get_command_buffer();

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vertexArray.vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buffer, vertexArray.index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

