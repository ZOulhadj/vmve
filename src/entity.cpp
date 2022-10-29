#include "entity.hpp"

#include "rendering/vulkan_renderer.hpp"

void Translate(EntityInstance* e, const glm::vec3& position)
{
    e->modelMatrix = glm::translate(e->modelMatrix, position);
    e->position = position;
}

void Rotate(EntityInstance* e, float deg, const glm::vec3& axis)
{
    e->modelMatrix = glm::rotate(e->modelMatrix, glm::radians(deg), axis);
}

void Scale(EntityInstance* e, float scale)
{
    e->modelMatrix = glm::scale(e->modelMatrix, glm::vec3(scale));
}

void Scale(EntityInstance* e, const glm::vec3& axis)
{
    e->modelMatrix = glm::scale(e->modelMatrix, axis);
}

glm::vec3 GetPosition(const EntityInstance* e)
{
#if 0
    // The position of an entity is encoded into the last column of the
    // transformation matrix so simply return that last column to get x, y and z.
    float x = e->modelMatrix[3].x;
    float y = e->modelMatrix[3].y;
    float z = e->modelMatrix[3].z;

    return { x, y, z };
#else

    return e->position;
#endif
}
