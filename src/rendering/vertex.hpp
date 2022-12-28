#ifndef MY_ENGINE_VERTEX_HPP
#define MY_ENGINE_VERTEX_HPP


struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    glm::vec3 tangent;

    float padding;
};


#endif
