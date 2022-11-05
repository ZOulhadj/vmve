#ifndef MY_ENGINE_VERTEXARRAY_HPP
#define MY_ENGINE_VERTEXARRAY_HPP


#include "Buffer.hpp"

struct VertexArray {
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

VertexArray CreateVertexArray(void* v, int vs, void* i, int is);
void destroy_vertex_array(VertexArray& vertexArray);

void bind_vertex_array(const VertexArray& vertexArray);

#endif
