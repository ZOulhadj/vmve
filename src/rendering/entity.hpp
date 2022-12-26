#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP



#include "rendering/vulkan/vertex_array.hpp"
#include "rendering/vulkan/texture.hpp"
#include "model.hpp"

struct instance_t
{  
    // TODO: Some of these member variables a here as we need to be able
    // to individually work with an instance. In the future, once instanced
    // rendering is implemented then the only data that should be sent is
    // the matrix.
    uint32_t id;
    std::string name;

    model_t* model;
    
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;


    glm::mat4 matrix = glm::mat4(1.0f);
};

void translate(instance_t& e, const glm::vec3& position);
void rotate(instance_t& e, float deg, const glm::vec3& axis);
void rotate(instance_t& e, const glm::vec3& axis);
void scale(instance_t& e, float scale);
void scale(instance_t& e, const glm::vec3& axis);
glm::vec3 get_position_from_matrix(const instance_t& e);
glm::vec3 get_scale_from_matrix(const instance_t& e);
glm::vec3 get_rotation_from_matrix(const instance_t& e);

#endif
