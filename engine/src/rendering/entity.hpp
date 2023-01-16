#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP



#include "rendering/vulkan/vertex_array.hpp"
#include "rendering/vulkan/texture.hpp"
#include "model.hpp"

struct Instance
{  
    // TODO: Some of these member variables a here as we need to be able
    // to individually work with an instance. In the future, once instanced
    // rendering is implemented then the only data that should be sent is
    // the matrix.
    uint32_t id;
    std::string name;

    uint32_t modelIndex;
    //Model* model;
    
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;


    glm::mat4 matrix = glm::mat4(1.0f);
};

void Translate(Instance& e, const glm::vec3& position);
void Rotate(Instance& e, float deg, const glm::vec3& axis);
void Rotate(Instance& e, const glm::vec3& axis);
void Scale(Instance& e, float scale);
void Scale(Instance& e, const glm::vec3& axis);
glm::vec3 GetPosFromMatrix(const Instance& e);
glm::vec3 GetScaleFromMatrix(const Instance& e);
glm::vec3 GetRotFromMatrix(const Instance& e);

void RenderModel(Model& model, glm::mat4& matrix, const std::vector<VkCommandBuffer>& cmdBuffer, VkPipelineLayout pipelineLayout);


#endif
