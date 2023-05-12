#ifndef MY_ENGINE_ENTITY_H
#define MY_ENGINE_ENTITY_H



#include "api/vulkan/vk_vertex_array.h"
#include "model.h"

namespace engine {

    // todo(zak): convert to using composition (ECS)
    struct Entity
    {
        // TODO: Some of these member variables a here as we need to be able
        // to individually work with an instance. In the future, once instanced
        // rendering is implemented then the only data that should be sent is
        // the matrix.
        uint32_t id;
        std::string name;

        uint32_t model_index;

        glm::mat4 matrix;
    };

    Entity create_entity(int id, int model_index, const std::string& name);
    void translate_entity(Entity& e, const glm::vec3& position);
    void rotate_entity(Entity& e, float deg, const glm::vec3& axis);
    void rotate_entity(Entity& e, const glm::vec3& axis);
    void scale_entity(Entity& e, float scale);
    void scale_entity(Entity& e, const glm::vec3& axis);

    // todo(zak): move this to either model.cpp or renderer.cpp
    void render_model(const Model_Old& model, const glm::mat4& matrix, const std::vector<VkCommandBuffer>& cmdBuffer, VkPipelineLayout pipelineLayout);
    void render_model(const Model_Old& model, const std::vector<VkCommandBuffer>& cmdBuffer, VkPipelineLayout pipelineLayout);


}

#endif
