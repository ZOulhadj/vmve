#include "Entity.hpp"

#include "Renderer/Renderer.hpp"

EntityInstance CreateEntity(VertexArray& model, TextureBuffer& texture, VkDescriptorSetLayout layout) {
    EntityInstance instance{};

    const RendererContext* rc = GetRendererContext();

    instance.matrix = glm::mat4(1.0f);
    instance.model = &model;
    instance.texture = &texture;
    instance.descriptorSet = AllocateDescriptorSet(layout);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = instance.descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &instance.texture->descriptor;

    vkUpdateDescriptorSets(rc->device.device, 1, &write, 0, nullptr);

    return instance;
}

void Translate(EntityInstance& e, const glm::vec3& position) {
    // todo: temp matrix reset
    e.matrix = glm::mat4(1.0f);

    e.matrix = glm::translate(e.matrix, position);
}

void Rotate(EntityInstance& e, float degrees, const glm::vec3& axis) {
    e.matrix = glm::rotate(e.matrix, glm::radians(degrees), axis);
}

void Rotate(EntityInstance& e, const glm::vec3& axis) {
    e.matrix = glm::rotate(e.matrix, glm::radians(axis.x), glm::vec3(1.0f, 0.0f, 0.0f));
    e.matrix = glm::rotate(e.matrix, glm::radians(axis.y), glm::vec3(0.0f, 1.0f, 0.0f));
    e.matrix = glm::rotate(e.matrix, glm::radians(axis.z), glm::vec3(0.0f, 0.0f, 1.0f));
}

void Scale(EntityInstance& e, float scale) {
    e.matrix = glm::scale(e.matrix, glm::vec3(scale));
}

void Scale(EntityInstance& e, const glm::vec3& axis) {
    e.matrix = glm::scale(e.matrix, axis);
}

glm::vec3 GetPosition(const EntityInstance& e) {
    return { e.matrix[3].x, e.matrix[3].y, e.matrix[3].z };
}
