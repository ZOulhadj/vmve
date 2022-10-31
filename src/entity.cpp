#include "entity.hpp"

void Translate(EntityInstance* e, const glm::vec3& position) {
    e->matrix = glm::translate(e->matrix, position);
}

void Rotate(EntityInstance* e, float deg, const glm::vec3& axis) {
    e->matrix = glm::rotate(e->matrix, glm::radians(deg), axis);
}

void Scale(EntityInstance* e, float scale) {
    e->matrix = glm::scale(e->matrix, glm::vec3(scale));
}

void Scale(EntityInstance* e, const glm::vec3& axis) {
    //e->scale = axis;
}

glm::vec3 GetPosition(const EntityInstance* e) {
    return { e->matrix[3].x, e->matrix[3].y, e->matrix[3].z };
}
