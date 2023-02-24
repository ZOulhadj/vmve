#ifndef MY_ENGINE_ENTITY_H
#define MY_ENGINE_ENTITY_H



#include "api/vulkan/vertex_array.h"
#include "model.h"

struct Entity
{  
    // TODO: Some of these member variables a here as we need to be able
    // to individually work with an instance. In the future, once instanced
    // rendering is implemented then the only data that should be sent is
    // the matrix.
    uint32_t id;
    std::string name;

    uint32_t model_index;
    //Model* model;
    
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::mat4 matrix;
};

Entity create_entity(int id, int model_index, const std::string& name, const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
void translate_entity(Entity& e, const glm::vec3& position);
void rotate_entity(Entity& e, float deg, const glm::vec3& axis);
void rotate_entity(Entity& e, const glm::vec3& axis);
void scale_entity(Entity& e, float scale);
void scale_entity(Entity& e, const glm::vec3& axis);

void apply_entity_transformation(Entity& e);

void decompose_entity_matrix(const float* matrix, float* pos, float* rot, float* scale);

// todo(zak): move this to either model.cpp or renderer.cpp
void render_model(Model& model, glm::mat4& matrix, const std::vector<VkCommandBuffer>& cmdBuffer, VkPipelineLayout pipelineLayout);
void render_model(Model& model, const std::vector<VkCommandBuffer>& cmdBuffer, VkPipelineLayout pipelineLayout);


#endif
