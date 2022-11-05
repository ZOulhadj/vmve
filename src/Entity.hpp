#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP


#include "Renderer/VertexArray.hpp"
#include "Renderer/Texture.hpp"


struct EntityInstance {
    glm::mat4 matrix;

    const VertexArray* model;
    const TextureBuffer* texture;

    VkDescriptorSet descriptorSet;
};


EntityInstance CreateEntity(VertexArray& model, TextureBuffer& texture, VkDescriptorSetLayout layout);

void Translate(EntityInstance& e, const glm::vec3& position);
void Rotate(EntityInstance& e, float deg, const glm::vec3& axis);
void Rotate(EntityInstance& e, const glm::vec3& axis);
void Scale(EntityInstance& e, float scale);
void Scale(EntityInstance& e, const glm::vec3& axis);
glm::vec3 GetPosition(const EntityInstance& e);

#endif
