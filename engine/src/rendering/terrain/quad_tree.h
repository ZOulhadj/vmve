#ifndef MY_ENGINE_QUAD_TREE_H
#define MY_ENGINE_QUAD_TREE_H

struct Quad_Tree_Node
{
    float     size;         // width and height
    glm::vec2 center;       // world space center

    Quad_Tree_Node* top_left;
    Quad_Tree_Node* top_right;
    Quad_Tree_Node* bottom_left;
    Quad_Tree_Node* bottom_right;

    // Actual data goes in here
    // Such as Objects

};

struct Quad_Tree
{
    Quad_Tree_Node* root_node;
    uint32_t        node_count;
};

Quad_Tree_Node* create_node(Quad_Tree* quad_tree, float size, glm::vec2& center);
void destroy_node(Quad_Tree_Node* root);
void split_node(Quad_Tree* quad_tree, Quad_Tree_Node* node);
Quad_Tree* create_quad_tree(float size, const glm::vec2& center);
void build_quad_tree(Quad_Tree* quad_tree, Quad_Tree_Node* node, const glm::vec2& position, float min_node_size);
void rebuild_quad_tree(Quad_Tree* quad_tree, float size, glm::vec2& center, float min_node_size, glm::vec2& position);


#endif