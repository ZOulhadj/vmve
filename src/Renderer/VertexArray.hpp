#ifndef MY_ENGINE_VERTEXARRAY_HPP
#define MY_ENGINE_VERTEXARRAY_HPP


#include "Buffer.hpp"

struct EntityModel {
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

EntityModel* CreateVertexArray(void* v, int vs, void* i, int is);
void DestroyVertexArray(EntityModel* model);

void BindVertexArray(const EntityModel* model);

#endif
