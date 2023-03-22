#ifndef MY_ENGINE_QUAD_TREE_H
#define MY_ENGINE_QUAD_TREE_H


struct Quad_Tree_Node
{
    glm::vec2 size;
    glm::vec3 position;

    std::array<Quad_Tree_Node*, 4> children;
};

struct Quad_Tree
{
    Quad_Tree_Node* root_node;
};

Quad_Tree* create_quad_tree(const glm::vec2& size, const glm::vec3& position);
void destroy_quad_tree(Quad_Tree* quad_tree);
void update_node(Quad_Tree_Node* node, const glm::vec2& min_node_size, const glm::vec3& position);
void rebuild_quad_tree(Quad_Tree* quad_tree, const glm::vec2& size, const glm::vec3& position, const glm::vec2& min_node_size, const glm::vec3& point);

Quad_Tree_Node* create_node(const glm::vec2& size, const glm::vec3& position);
void destroy_node(Quad_Tree_Node* node);
void subdivide_node(Quad_Tree_Node* node);

// temp

void visualise_node(Quad_Tree_Node* node);

#endif