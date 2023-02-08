#ifndef MY_ENGINE_VERTEX_ARRAY_HPP
#define MY_ENGINE_VERTEX_ARRAY_HPP


#include "buffer.hpp"
#include "rendering/vertex.hpp"

struct Vertex_Array {
    vulkan_buffer   vertex_buffer;
    vulkan_buffer   index_buffer;
    uint32_t index_count;
};

Vertex_Array create_vertex_array(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
void destroy_vertex_array(Vertex_Array& vertexArray);

void bind_vertex_array(const std::vector<VkCommandBuffer>& buffers, const Vertex_Array& vertex_array);

#endif
