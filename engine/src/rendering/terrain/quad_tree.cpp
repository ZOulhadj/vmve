#include "pch.h"
#include "quad_tree.h"


#include "utility.h"

Quad_Tree* create_quad_tree(const glm::vec2& size, const glm::vec3& position)
{
    Quad_Tree* quad_tree = new Quad_Tree();

    quad_tree->root_node = create_node(size, position);

    return quad_tree;
}

void destroy_quad_tree(Quad_Tree* quad_tree)
{
    if (!quad_tree)
        return;

    destroy_node(quad_tree->root_node);

    delete quad_tree;
}

void update_node(Quad_Tree_Node* node, const glm::vec2& min_node_size, const glm::vec3& position)
{
    // TODO: If not updating a leaf node then we must first remove free all children
    // TODO: Understand why we do position.z + instead of -. forward and backwards is
    // in reverse if - is used.
    const float distance_to_center = std::sqrt(std::pow(node->position.x - position.x, 2.0f) +
                                               std::pow(node->position.y - position.y, 2.0f) +
                                               std::pow(node->position.z + position.z, 2.0f));
    const bool too_close        = distance_to_center < node->size.x || distance_to_center < node->size.y;
    const bool above_size_limit = node->size.x > min_node_size.x && node->size.y > min_node_size.y;

    if (too_close && above_size_limit) {
        subdivide_node(node);

        for (auto& child_node : node->children) {
            update_node(child_node, min_node_size, position);
        }
    }
}

void rebuild_quad_tree(Quad_Tree* quad_tree, const glm::vec2& size, const glm::vec3& position, const glm::vec2& min_node_size, const glm::vec3& point)
{
    destroy_node(quad_tree->root_node);
    quad_tree->root_node = create_node(size, position);

    update_node(quad_tree->root_node, min_node_size, point);
}


Quad_Tree_Node* create_node(const glm::vec2& size, const glm::vec3& position)
{
    Quad_Tree_Node* node = new Quad_Tree_Node();
    node->size = size;
    node->position = position;

    return node;
}

void destroy_node(Quad_Tree_Node* node)
{
    if (!node)
        return;

    for (Quad_Tree_Node* child_node : node->children)
        destroy_node(child_node);

    delete node;

    // delete does not set memory address to nullptr
    node = nullptr;
}


void subdivide_node(Quad_Tree_Node* node)
{
    // Calculate the center positions for each child node
    const glm::vec2 offset = node->size / 4.0f;

    // Construct center positions for each child node
    const std::array<glm::vec3, 4> centers {
        glm::vec3(node->position.x - offset.x, node->position.y, node->position.z + offset.y), // top left
        glm::vec3(node->position.x + offset.x, node->position.y, node->position.z + offset.y), // top right
        glm::vec3(node->position.x + offset.x, node->position.y, node->position.z - offset.y), // bottom right
        glm::vec3(node->position.x - offset.x, node->position.y, node->position.z - offset.y)  // bottom left
    };

    // Create children nodes
    for (std::size_t i = 0; i < node->children.size(); ++i) {
        node->children[i] = create_node(node->size / 2.0f, centers[i]);
    }
}

void visualise_node(Quad_Tree_Node* node)
{
    if (!node)
        return;

    for (Quad_Tree_Node* child_node : node->children)
        visualise_node(child_node);

    const glm::vec2 offset = node->size / 2.0f;

    const ImVec2 min_corner = ImVec2(node->position.x - offset.x, node->position.z + offset.y);
    const ImVec2 max_corner = ImVec2(node->position.x + offset.x, node->position.z - offset.y);

    ImVec2 p_min = min_corner + ImGui::GetMainViewport()->GetCenter();
    ImVec2 p_max = max_corner + ImGui::GetMainViewport()->GetCenter();

    ImGui::GetForegroundDrawList()->AddRect(p_min, p_max, ImGui::ColorConvertFloat4ToU32({ 1.0f, 1.0f, 1.0f, 1.0f }));




    //ImVec2 line_start = ImVec2(node->center.x, node->center.y) + ImGui::GetMainViewport()->GetCenter();
    //ImVec2 line_end   = ImVec2(camera.position.x, camera.position.y) + ImGui::GetMainViewport()->GetCenter();
    //ImGui::GetForegroundDrawList()->AddLine(line_start, line_end, ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)));
}
