#ifndef MY_ENGINE_VERTEX_ARRAY_HPP
#define MY_ENGINE_VERTEX_ARRAY_HPP


#include "buffer.hpp"

struct vertex_array_t {
    buffer_t   vertex_buffer;
    buffer_t   index_buffer;
    uint32_t index_count;
};

vertex_array_t create_vertex_array(void* v, int vs, void* i, int is);
void destroy_vertex_array(vertex_array_t& vertexArray);

void bind_vertex_array(VkCommandBuffer cmd_buffer, const vertex_array_t& vertexArray);

#endif
