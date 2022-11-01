#ifndef MY_ENGINE_VERTEXARRAY_HPP
#define MY_ENGINE_VERTEXARRAY_HPP


#include "Buffer.hpp"

struct VertexArray {
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

VertexArray CreateVertexArray(void* v, int vs, void* i, int is);
void DestroyVertexArray(VertexArray& vertexArray);

void BindVertexArray(const VertexArray& vertexArray);

#endif
