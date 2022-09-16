#include "entity.hpp"

#include "vulkan_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

void translate_entity(entity* e, float x, float y, float z)
{
    e->model = glm::translate(e->model, { x, y, z });
}

void rotate_entity(entity* e, float deg, float x, float y, float z)
{
    e->model = glm::rotate(e->model, glm::radians(deg), { x, y, z });
}

void scale_entity(entity* e, float scale)
{
    e->model = glm::scale(e->model, { scale, scale, scale });
}

void scale_entity(entity* e, float x, float y, float z)
{
    e->model = glm::scale(e->model, { x, y, z });
}

void get_entity_position(const entity* e, float* x, float* y, float* z)
{
    // The position of an entity is encoded into the last column of the model
    // matrix so simply return that last column to get x, y and z.
    *x = e->model[3].x;
    *y = e->model[3].y;
    *z = e->model[3].z;
}
