#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP

struct VertexBuffer;
struct TextureBuffer;

struct Entity
{
    glm::mat4 model;

    const VertexBuffer* vertex_buffer;
    const TextureBuffer* texture_buffer;

    VkDescriptorSet descriptor_set;
};

void translate_entity(Entity* e, float x, float y, float z);
void rotate_entity(Entity* e, float deg, float x, float y, float z);
void scale_entity(Entity* e, float scale);
void scale_entity(Entity* e, float x, float y, float z);
void get_entity_position(const Entity* e, float* x, float* y, float* z);

#endif
