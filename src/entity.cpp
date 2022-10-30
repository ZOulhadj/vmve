#include "entity.hpp"

void Translate(EntityInstance* e, const glm::vec3& position) {
    e->position = position;
}

void Rotate(EntityInstance* e, float deg, const glm::vec3& axis) {
}

void Scale(EntityInstance* e, float scale) {
    e->scale = scale;
}

void Scale(EntityInstance* e, const glm::vec3& axis) {
    //e->scale = axis;
}

glm::vec3 GetPosition(const EntityInstance* e) {
    return e->position;
}
