#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP

struct EntityModel;
struct EntityTexture;

struct EntityInstance {
    glm::vec3 position;
    glm::vec3 rotation;
    float     scale;

    EntityModel* model;
    EntityTexture* texture;

    VkDescriptorSet descriptorSet;
};

void Translate(EntityInstance* e, const glm::vec3& position);
void Rotate(EntityInstance* e, float deg, const glm::vec3& axis);
void Scale(EntityInstance* e, float scale);
void Scale(EntityInstance* e, const glm::vec3& axis);
glm::vec3 GetPosition(const EntityInstance* e);

#endif
