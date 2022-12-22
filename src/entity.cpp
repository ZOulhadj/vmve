#include "entity.hpp"

void translate(instance_t& e, const glm::vec3& position)
{
    // todo: temp matrix reset
    e.matrix = glm::mat4(1.0f);
    e.matrix = glm::translate(e.matrix, position);

    e.position = position;
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

glm::vec3 get_position_from_matrix(const instance_t& e)
{
    return { e.matrix[3].x, e.matrix[3].y, e.matrix[3].z };
}

glm::vec3 get_scale_from_matrix(const instance_t& e)
{
    //const float x1 = glm::pow(e.matrix[0].x, 2);
    //const float x2 = glm::pow(e.matrix[0].y, 2);
    //const float x3 = glm::pow(e.matrix[0].z, 2);

    //const float y1 = glm::pow(e.matrix[1].x, 2);
    //const float y2 = glm::pow(e.matrix[1].y, 2);
    //const float y3 = glm::pow(e.matrix[1].z, 2);

    //const float z1 = glm::pow(e.matrix[2].x, 2);
    //const float z2 = glm::pow(e.matrix[2].y, 2);
    //const float z3 = glm::pow(e.matrix[2].z, 2);

    //const float x = glm::sqrt(x1 + x2 + x3);
    //const float y = glm::sqrt(y1 + y2 + y3);
    //const float z = glm::sqrt(z1 + z2 + z3);

    //return { x, y, z };

    return glm::vec3(0.0f);
}

glm::vec3 get_rotation_from_matrix(const instance_t& e)
{
    /*return { e.matrix[0].x, e.matrix[1].y, e.matrix[2].z };*/

    return glm::vec3(0.0f);
}
