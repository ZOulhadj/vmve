#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP

struct vertex_buffer;

struct entity
{
    glm::mat4 model;

    const vertex_buffer* vertexBuffer;
};

void translate_entity(entity* e, float x, float y, float z);
void rotate_entity(entity* e, float deg, float x, float y, float z);
void scale_entity(entity* e, float scale);
void scale_entity(entity* e, float x, float y, float z);
void get_entity_position(const entity* e, float* x, float* y, float* z);

#endif
