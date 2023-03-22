#include "pch.h"
#include "quad_tree.h"


Quad_Tree_Node* create_node(Quad_Tree* quad_tree, float size, const glm::vec2& center)
{
    Quad_Tree_Node* node = new Quad_Tree_Node();
    node->size = size;
    node->center = center;

    quad_tree->node_count++;

    return node;
}

void destroy_node(Quad_Tree_Node* root)
{
    if (!root)
        return;

    destroy_node(root->top_left);
    destroy_node(root->top_right);
    destroy_node(root->bottom_right);
    destroy_node(root->bottom_left);


    delete root;
}


void split_node(Quad_Tree* quad_tree, Quad_Tree_Node* node)
{
    // Calculate the center positions for each child node
    const float quater_size = node->size / 4.0f;
    const glm::vec2 top_left     = { node->center.x - quater_size, node->center.y - quater_size };
    const glm::vec2 top_right    = { node->center.x + quater_size, node->center.y - quater_size };
    const glm::vec2 bottom_right = { node->center.x + quater_size, node->center.y + quater_size };
    const glm::vec2 bottom_left  = { node->center.x - quater_size, node->center.y + quater_size };

    // Create children nodes
    node->top_left     = create_node(quad_tree, node->size / 2.0f, top_left);
    node->top_right    = create_node(quad_tree, node->size / 2.0f, top_right);
    node->bottom_right = create_node(quad_tree, node->size / 2.0f, bottom_right);
    node->bottom_left  = create_node(quad_tree, node->size / 2.0f, bottom_left);

}

Quad_Tree* create_quad_tree(float size, const glm::vec2& center)
{
    Quad_Tree* quad_tree = new Quad_Tree();

    quad_tree->node_count = 0;
    quad_tree->root_node  = create_node(quad_tree, size, center);

    return quad_tree;
}

void build_quad_tree(Quad_Tree* quad_tree, Quad_Tree_Node* node, const glm::vec2& position, float min_node_size)
{
    const float dist_to_center = std::sqrt(std::pow(node->center.x - position.x, 2.0f) +
                                           std::pow(node->center.y - position.y, 2.0f));

    if (dist_to_center < node->size && node->size > min_node_size) {
        split_node(quad_tree, node);

        build_quad_tree(quad_tree, node->top_left, position, min_node_size);
        build_quad_tree(quad_tree, node->top_right, position, min_node_size);
        build_quad_tree(quad_tree, node->bottom_right, position, min_node_size);
        build_quad_tree(quad_tree, node->bottom_left, position, min_node_size);
    }
}

void rebuild_quad_tree(Quad_Tree* quad_tree, float size, const glm::vec2& center, float min_node_size, const glm::vec2& position)
{
    destroy_node(quad_tree->root_node);

    quad_tree->node_count = 0;
    quad_tree->root_node = create_node(quad_tree, size, center);

    build_quad_tree(quad_tree, quad_tree->root_node, position, min_node_size);
}