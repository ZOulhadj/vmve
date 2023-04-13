#include "pch.h"
#include "entity.h"

#include "api/vulkan/vk_renderer.h"

namespace engine {
    Entity create_entity(int id, int model_index, const std::string& name)
    {
        Entity entity{};
        entity.id = id;
        entity.name = name;
        entity.model_index = model_index;
        entity.matrix = glm::mat4(1.0f);

        print_log("Entity %s with ID (%d) created.\n", entity.name.c_str(), entity.id);

        return entity;
    }


    void translate_entity(Entity& e, const glm::vec3& position)
    {
        // ensure that before translating the matrix is an identity
        //assert(e.matrix == glm::identity<glm::mat4>());

        e.matrix = glm::translate(e.matrix, position);
    }

    void rotate_entity(Entity& e, float deg, const glm::vec3& axis)
    {
        e.matrix = glm::rotate(e.matrix, glm::radians(deg), axis);
    }

    void rotate_entity(Entity& e, const glm::vec3& axis)
    {
        if (axis.x > 0.0f)
            e.matrix = glm::rotate(e.matrix, glm::radians(axis.x), glm::vec3(1.0f, 0.0f, 0.0f));
        if (axis.y > 0.0f)
            e.matrix = glm::rotate(e.matrix, glm::radians(axis.y), glm::vec3(0.0f, 1.0f, 0.0f));
        if (axis.z > 0.0f)
            e.matrix = glm::rotate(e.matrix, glm::radians(axis.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    void scale_entity(Entity& e, float scale)
    {
        e.matrix = glm::scale(e.matrix, glm::vec3(scale));
    }

    void scale_entity(Entity& e, const glm::vec3& axis)
    {
        e.matrix = glm::scale(e.matrix, axis);
    }

    void render_model(Model& model, glm::mat4& matrix, const std::vector<VkCommandBuffer>& cmdBuffer, VkPipelineLayout pipelineLayout)
    {
        for (std::size_t i = 0; i < model.meshes.size(); ++i) {
            bind_descriptor_set(cmdBuffer, pipelineLayout, model.meshes[i].descriptor_set);
            bind_vertex_array(cmdBuffer, model.meshes[i].vertex_array);
            render(cmdBuffer, pipelineLayout, model.meshes[i].vertex_array.index_count, matrix);
        }
    }

    void render_model(Model& model, const std::vector<VkCommandBuffer>& cmdBuffer, VkPipelineLayout pipelineLayout)
    {
        for (std::size_t i = 0; i < model.meshes.size(); ++i) {
            bind_descriptor_set(cmdBuffer, pipelineLayout, model.meshes[i].descriptor_set);
            bind_vertex_array(cmdBuffer, model.meshes[i].vertex_array);
            render(cmdBuffer, model.meshes[i].vertex_array.index_count);
        }
    }
}


