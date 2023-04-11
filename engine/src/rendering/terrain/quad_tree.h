#ifndef MY_ENGINE_QUAD_TREE_H
#define MY_ENGINE_QUAD_TREE_H

namespace engine {
    enum Quad_Tree_Node_Location
    {
        top_left,
        top_right,
        bottom_right,
        bottom_left
    };

    struct Quad_Tree_Node
    {
        glm::vec2 size;
        glm::vec3 pos;

        std::array<Quad_Tree_Node*, 4> children;
    };

    struct Quad_Tree
    {
        Quad_Tree_Node* root_node;
        glm::vec2       min_size;

        uint32_t        node_count;
    };

    Quad_Tree* create_quad_tree(const glm::vec2& size, const glm::vec2& min_size, const glm::vec3& position);
    void destroy_quad_tree(Quad_Tree* quad_tree);
    Quad_Tree* rebuild_quad_tree(Quad_Tree* quad_tree);

    void insert_point(Quad_Tree* quad_tree, const glm::vec3& position);


    // temp

    void visualise_node(Quad_Tree_Node* node, float scale);
    void visualise_terrain(Quad_Tree_Node* node, float scale, int depth = 0);
}


#endif