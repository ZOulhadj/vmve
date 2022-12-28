#ifndef MY_ENGINE_VERTEX_ARRAY_HPP
#define MY_ENGINE_VERTEX_ARRAY_HPP


#include "buffer.hpp"
#include "../vertex.hpp"

struct VertexArray {
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

VertexArray CreateVertexArray(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
void DestroyVertexArray(VertexArray& vertexArray);

void bind_vertex_array(std::vector<VkCommandBuffer>& buffers, const VertexArray& vertexArray);

#endif
