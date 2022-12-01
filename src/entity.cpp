#include "entity.hpp"

void translate(instance_t& e, const glm::vec3& position)
{
    // todo: temp matrix reset
    e.matrix = glm::mat4(1.0f);
    e.matrix = glm::translate(e.matrix, position);
}

void rotate(instance_t& e, float deg, const glm::vec3& axis)
{
    e.matrix = glm::rotate(e.matrix, glm::radians(deg), axis);
}

void rotate(instance_t& e, const glm::vec3& axis)
{
    e.matrix = glm::rotate(e.matrix, glm::radians(axis.x), glm::vec3(1.0f, 0.0f, 0.0f));
    e.matrix = glm::rotate(e.matrix, glm::radians(axis.y), glm::vec3(0.0f, 1.0f, 0.0f));
    e.matrix = glm::rotate(e.matrix, glm::radians(axis.z), glm::vec3(0.0f, 0.0f, 1.0f));
}

void scale(instance_t& e, float scale)
{
    e.matrix = glm::scale(e.matrix, glm::vec3(scale));
}

void scale(instance_t& e, const glm::vec3& axis)
{
    e.matrix = glm::scale(e.matrix, axis);
}

glm::vec3 get_position(const instance_t& e)
{
    return { e.matrix[3].x, e.matrix[3].y, e.matrix[3].z };
}
