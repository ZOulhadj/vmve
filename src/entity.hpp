#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP


#include "renderer/vertex_array.hpp"
#include "renderer/texture.hpp"

struct instance_t
{
    glm::mat4 matrix;
};

void translate(instance_t& e, const glm::vec3& position);
void rotate(instance_t& e, float deg, const glm::vec3& axis);
void rotate(instance_t& e, const glm::vec3& axis);
void scale(instance_t& e, float scale);
void scale(instance_t& e, const glm::vec3& axis);
glm::vec3 get_position(const instance_t& e);

#endif
