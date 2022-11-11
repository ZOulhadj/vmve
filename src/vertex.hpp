#ifndef MY_ENGINE_VERTEX_HPP
#define MY_ENGINE_VERTEX_HPP


struct vertex_t {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    glm::vec3 tangent;
    glm::vec3 biTangent;

    float padding;
};


#endif
