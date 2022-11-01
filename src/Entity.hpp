#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP


#include "Renderer/VertexArray.hpp"
#include "Renderer/Texture.hpp"


struct EntityInstance {
    glm::mat4 matrix;

    EntityModel* model;
    EntityTexture* texture;

    VkDescriptorSet descriptorSet;
};


EntityInstance* CreateEntity(EntityModel* model, EntityTexture* texture, VkDescriptorSetLayout layout);
void DestroyEntity(EntityInstance* instance);

void Translate(EntityInstance* e, const glm::vec3& position);
void Rotate(EntityInstance* e, float deg, const glm::vec3& axis);
void Scale(EntityInstance* e, float scale);
void Scale(EntityInstance* e, const glm::vec3& axis);
glm::vec3 GetPosition(const EntityInstance* e);

#endif
