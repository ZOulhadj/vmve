#ifndef MYENGINE_ENTITY_HPP
#define MYENGINE_ENTITY_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


struct VertexBuffer;

struct Entity
{
    glm::mat4 model;

    const VertexBuffer* vertexBuffer;
};

#endif
