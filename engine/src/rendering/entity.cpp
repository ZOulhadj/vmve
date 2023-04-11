#include "pch.h"
#include "entity.h"

#include "api/vulkan/vk_renderer.h"

namespace engine {
    Entity create_entity(int id, int model_index, const std::string& name, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
    {
        Entity entity{};
        entity.id = id;
        entity.name = name;
        entity.model_index = model_index;
        entity.position = pos;
        entity.rotation = rot;
        entity.scale = scale;
        entity.matrix = glm::mat4(1.0f);

        print_log("Entity %s with ID (%d) created.\n", entity.name.c_str(), entity.id);

        return entity;
    }


    void translate_entity(Entity& e, const glm::vec3& position)
    {
        // todo: temp matrix reset
        //e.matrix = glm::mat4(1.0f);
        //e.matrix = glm::translate(e.matrix, position);

        e.position = position;
    }

    void rotate_entity(Entity& e, float deg, const glm::vec3& axis)
    {
        //e.matrix = glm::rotate(e.matrix, glm::radians(deg), axis);

        e.rotation = axis * deg;
    }

    void rotate_entity(Entity& e, const glm::vec3& axis)
    {
        //e.matrix = glm::rotate(e.matrix, glm::radians(axis.x), glm::vec3(1.0f, 0.0f, 0.0f));
        //e.matrix = glm::rotate(e.matrix, glm::radians(axis.y), glm::vec3(0.0f, 1.0f, 0.0f));
        //e.matrix = glm::rotate(e.matrix, glm::radians(axis.z), glm::vec3(0.0f, 0.0f, 1.0f));

        e.rotation = axis;
    }

    void scale_entity(Entity& e, float scale)
    {
        e.scale = glm::vec3(scale);
        //e.matrix = glm::scale(e.matrix, glm::vec3(scale));
    }

    void scale_entity(Entity& e, const glm::vec3& axis)
    {
        e.scale = glm::vec3(axis.x, axis.y, axis.z);
        //e.matrix = glm::scale(e.matrix, axis);
    }

    void apply_entity_transformation(Entity& e)
    {
        e.matrix = glm::mat4(1.0f);

        e.matrix = glm::translate(e.matrix, e.position);
        e.matrix = glm::scale(e.matrix, e.scale);

        // TODO: Figure out how to rotate object without needing to rotate axis that are not used.
        //e.matrix = glm::rotate(e.matrix, glm::radians(e.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        //e.matrix = glm::rotate(e.matrix, glm::radians(e.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        //e.matrix = glm::rotate(e.matrix, glm::radians(e.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }


    void get_entity_position(Entity& e, float* pos)
    {
        pos[0] = e.position.x;
        pos[0] = e.position.y;
        pos[0] = e.position.z;
    }

    void get_entity_rotation(Entity& e, float* rot)
    {
        rot[0] = e.rotation.x;
        rot[1] = e.rotation.y;
        rot[2] = e.rotation.z;
    }

    void get_entity_scale(Entity& e, float* scale)
    {
        scale[0] = e.scale.x;
        scale[1] = e.scale.y;
        scale[2] = e.scale.z;
    }

    void decompose_entity_matrix(const float* matrix, float* pos, float* rot, float* scale)
    {
        glm::mat4 mat = *(glm::mat4*)matrix;

        scale[0] = glm::length(mat[0]);
        scale[1] = glm::length(mat[1]);
        scale[2] = glm::length(mat[2]);


        glm::vec3 r = glm::vec3(mat[0][0], mat[0][1], mat[0][1]) / scale[0];
        print_log("%s\n", glm::to_string(r).c_str());

#if 1
        rot[0] = r.x;
        rot[1] = r.y;
        rot[2] = r.z;
#else
        rot[0] = glm::length(mat[0]) > FLT_EPSILON ? glm::vec4(1.0f / glm::length(mat[0])) : glm::vec4(1.0f / FLT_EPSILON);
        rot[1] = glm::length(mat[1]) > FLT_EPSILON ? glm::vec4(1.0f / glm::length(mat[1])) : glm::vec4(1.0f / FLT_EPSILON);
        rot[2] = glm::length(mat[2]) > FLT_EPSILON ? glm::vec4(1.0f / glm::length(mat[2])) : glm::vec4(1.0f / FLT_EPSILON);
#endif

        pos[0] = mat[3][0];
        pos[1] = mat[3][1];
        pos[2] = mat[3][2];
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


