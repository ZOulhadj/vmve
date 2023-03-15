#ifndef MY_ENGINE_VULKAN_VERTEX_ARRAY_H
#define MY_ENGINE_VULKAN_VERTEX_ARRAY_H


#include "vk_buffer.h"
#include "rendering/vertex.h"

struct vk_vertex_array
{
    vk_buffer vertex_buffer;
    vk_buffer index_buffer;
    uint32_t  index_count;
};

vk_vertex_array create_vertex_array(const std::vector<vertex>& vertices, const std::vector<uint32_t>& indices);
void destroy_vertex_array(vk_vertex_array& vertexArray);

void bind_vertex_array(const std::vector<VkCommandBuffer>& buffers, const vk_vertex_array& vertex_array);

#endif
