#include "Entity.hpp"

#include "Renderer/Renderer.hpp"

EntityInstance* CreateEntity(EntityModel* model, EntityTexture* texture, VkDescriptorSetLayout layout) {
    auto e = new EntityInstance();

    const RendererContext* rc = GetRendererContext();

    e->matrix = glm::mat4(1.0f);
    e->model = model;
    e->texture = texture;
    e->descriptorSet = AllocateDescriptorSet(layout);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = e->descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &e->texture->descriptor;

    vkUpdateDescriptorSets(rc->device.device, 1, &write, 0, nullptr);

    return e;
}

void DestroyEntity(EntityInstance* instance) {
    delete instance;
}

void Translate(EntityInstance* e, const glm::vec3& position) {
    e->matrix = glm::translate(e->matrix, position);
}

void Rotate(EntityInstance* e, float deg, const glm::vec3& axis) {
    e->matrix = glm::rotate(e->matrix, glm::radians(deg), axis);
}

void Scale(EntityInstance* e, float scale) {
    e->matrix = glm::scale(e->matrix, glm::vec3(scale));
}

void Scale(EntityInstance* e, const glm::vec3& axis) {
    //e->scale = axis;
}

glm::vec3 GetPosition(const EntityInstance* e) {
    return { e->matrix[3].x, e->matrix[3].y, e->matrix[3].z };
}
