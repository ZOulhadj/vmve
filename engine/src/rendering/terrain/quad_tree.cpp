#include "pch.h"
#include "quad_tree.h"

#include "utils/logging.h"


namespace engine {
    static Quad_Tree_Node* create_node(Quad_Tree* quad_tree, const glm::vec2& size, const glm::vec3& position)
    {
        Quad_Tree_Node* node = new Quad_Tree_Node();
        node->size = size;
        node->pos = position;

        quad_tree->node_count++;

        return node;
    }

    static void destroy_node(Quad_Tree* quad_tree, Quad_Tree_Node* node)
    {
        if (!node)
            return;

        destroy_node(quad_tree, node->children[top_left]);
        destroy_node(quad_tree, node->children[top_right]);
        destroy_node(quad_tree, node->children[bottom_right]);
        destroy_node(quad_tree, node->children[bottom_left]);

        delete node;

        quad_tree->node_count--;
    }

    static bool is_leaf_node(Quad_Tree_Node* node)
    {
        // Only need to check one since all children will be present if not null
        return node->children[0] == nullptr;
    }

    static bool node_contains(Quad_Tree_Node* node, const glm::vec3& position)
    {
        const glm::vec2 offset = node->size * 0.50f;
        const glm::vec2 min_corner = glm::vec2(node->pos.x - offset.x, node->pos.z + offset.y);
        const glm::vec2 max_corner = glm::vec2(node->pos.x + offset.x, node->pos.z - offset.y);

        return min_corner.x <= position.x && min_corner.y >= position.z &&
            max_corner.x >= position.x && max_corner.y <= position.z;
    }

    static void subdivide_node(Quad_Tree* quad_tree, Quad_Tree_Node* node, const glm::vec3& position)
    {
        // helper variables
        const glm::vec2 size = node->size;
        const glm::vec3 pos = node->pos;

        // TODO: If not updating a leaf node then we must first remove free all children
        // TODO: Understand why we do position.z + instead of -. forward and backwards is
        // in reverse if - is used.

        const float distance_to_center = glm::sqrt(glm::pow(pos.x - position.x, 2.0f) +
            glm::pow(pos.z + position.z, 2.0f));
        const bool too_close = distance_to_center < size.x || distance_to_center < size.y;
        const bool above_size_limit = size.x > quad_tree->min_size.x && size.y > quad_tree->min_size.y;

        if (too_close && above_size_limit) {
            // This is required for multiple points. If another point is to be inserted
            // into the quad tree and children for a particular node already exists then
            // we must not allocate new nodes. Instead, use the existing nodes and traverse
            // further down.
            if (is_leaf_node(node)) {
                // Create children nodes
                const glm::vec2 offset = size * 0.25f;
                const glm::vec2 new_size = size * 0.50f;


                node->children[top_left] = create_node(quad_tree, new_size, { pos.x - offset.x, pos.y, pos.z + offset.y });
                node->children[top_right] = create_node(quad_tree, new_size, { pos.x + offset.x, pos.y, pos.z + offset.y });
                node->children[bottom_right] = create_node(quad_tree, new_size, { pos.x + offset.x, pos.y, pos.z - offset.y });
                node->children[bottom_left] = create_node(quad_tree, new_size, { pos.x - offset.x, pos.y, pos.z - offset.y });
            }


            subdivide_node(quad_tree, node->children[top_left], position);
            subdivide_node(quad_tree, node->children[top_right], position);
            subdivide_node(quad_tree, node->children[bottom_right], position);
            subdivide_node(quad_tree, node->children[bottom_left], position);
        }




        // Set data or something
    }

    Quad_Tree* create_quad_tree(const glm::vec2& size, const glm::vec2& min_size, const glm::vec3& position)
    {
        Quad_Tree* quad_tree = new Quad_Tree();
        quad_tree->root_node = create_node(quad_tree, size, position);
        quad_tree->min_size = min_size;

        return quad_tree;
    }

    void destroy_quad_tree(Quad_Tree* quad_tree)
    {
        if (!quad_tree)
            return;

        destroy_node(quad_tree, quad_tree->root_node);

        delete quad_tree;
    }

    Quad_Tree* rebuild_quad_tree(Quad_Tree* quad_tree)
    {
        // todo: only update nodes where the camera is located

        auto root_size = quad_tree->root_node->size;
        auto position = quad_tree->root_node->pos;
        auto min_size = quad_tree->min_size;
        destroy_quad_tree(quad_tree);

        return create_quad_tree(root_size, min_size, position);
    }

    void insert_point(Quad_Tree* quad_tree, const glm::vec3& position)
    {
        subdivide_node(quad_tree, quad_tree->root_node, position);
    }

    void visualise_node(Quad_Tree_Node* node, float scale)
    {
        if (!node)
            return;


        visualise_node(node->children[top_left], scale);
        visualise_node(node->children[top_right], scale);
        visualise_node(node->children[bottom_right], scale);
        visualise_node(node->children[bottom_left], scale);

        const glm::vec2 offset = node->size * 0.50f;

        const ImVec2 min_corner = ImVec2(node->pos.x - offset.x, node->pos.z + offset.y) * scale;
        const ImVec2 max_corner = ImVec2(node->pos.x + offset.x, node->pos.z - offset.y) * scale;

        ImVec2 p_min = min_corner + ImGui::GetMainViewport()->GetCenter();
        ImVec2 p_max = max_corner + ImGui::GetMainViewport()->GetCenter();

        ImGui::GetForegroundDrawList()->AddRect(p_min, p_max, ImGui::ColorConvertFloat4ToU32({ 1.0f, 1.0f, 1.0f, 1.0f }));


        //ImVec2 line_start = ImVec2(node->center.x, node->center.y) + ImGui::GetMainViewport()->GetCenter();
        //ImVec2 line_end   = ImVec2(camera.position.x, camera.position.y) + ImGui::GetMainViewport()->GetCenter();
        //ImGui::GetForegroundDrawList()->AddLine(line_start, line_end, ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)));
    }

    void visualise_terrain(Quad_Tree_Node* node, float scale, int depth)
    {
        if (!node)
            return;

        visualise_terrain(node->children[top_left], scale, depth + 1);
        visualise_terrain(node->children[top_right], scale, depth + 1);
        visualise_terrain(node->children[bottom_right], scale, depth + 1);
        visualise_terrain(node->children[bottom_left], scale, depth + 1);

        // Only render chunks for leaf nodes
        if (is_leaf_node(node)) {
            const glm::vec2 offset = node->size * 0.50f;

            const ImVec2 min_corner = ImVec2(node->pos.x - offset.x, node->pos.z + offset.y) * scale;
            const ImVec2 max_corner = ImVec2(node->pos.x + offset.x, node->pos.z - offset.y) * scale;
            const ImVec2 p_min = min_corner + ImGui::GetMainViewport()->GetCenter();
            const ImVec2 p_max = max_corner + ImGui::GetMainViewport()->GetCenter();

            // TODO: This is the chunk falloff and should be fine tuned.
            const int chunk_count = 1 * depth + 1;


            // TODO: Chunk generation occurs here and should not be recreated each frame.
            // Only if the parent nodes needs updating.
            for (int z = 0; z < chunk_count; ++z) {
                for (int x = 0; x < chunk_count; ++x) {
                    const float step_count = (node->size.x / chunk_count) * scale;

                    // TODO: Needs to be looked at closer as this is what positions the chunks
                    // within each quad tree node
                    float min_x = p_min.x + (step_count * x);
                    float max_x = min_x + step_count;

                    float min_y = p_min.y - (step_count * z);
                    float max_y = min_y - step_count;

                    // Render the chunks
                    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(min_x, min_y), ImVec2(max_x, max_y),
                        ImGui::ColorConvertFloat4ToU32({ glm::sin((float)depth), 0.2f, glm::cos((float)depth), 0.3f }));
                    ImGui::GetForegroundDrawList()->AddRect(ImVec2(min_x, min_y), ImVec2(max_x, max_y),
                        ImGui::ColorConvertFloat4ToU32({ glm::sin((float)depth), 0.2f, glm::cos((float)depth), 0.9f }));
                }
            }
        }

    }
}





#if 0
Model terrain_model{};
terrain_model.name = "terrain";

Mesh terrain_mesh{};

float terrain_size = 1.0f;
float terrain_scale = 500.0f;
for (float z = -(terrain_size / 2.0f); z <= terrain_size / 2.0f; ++z) {
    for (float x = -(terrain_size / 2.0f); x <= terrain_size / 2.0f; ++x) {
        vertex v{};
        v.position = { x * terrain_scale, 0.0f, z * terrain_scale };
        v.normal = { 0.0f, 1.0f, 0.0f };

        terrain_mesh.vertices.push_back(v);
    }
}


// indices
int i = 0;
for (float z = 0; z < terrain_size; ++z, ++i) {
    for (float x = 0; x < terrain_size; ++x, ++i) {
        int topLeft = terrain_size + i + 1;
        int topRight = topLeft + 1;
        int bottomLeft = i;
        int bottomRight = bottomLeft + 1;

        terrain_mesh.indices.push_back(bottomRight);
        terrain_mesh.indices.push_back(bottomLeft);
        terrain_mesh.indices.push_back(topRight);


        terrain_mesh.indices.push_back(topLeft);
        terrain_mesh.indices.push_back(topRight);
        terrain_mesh.indices.push_back(bottomLeft);
    }
}


create_fallback_albedo_texture(terrain_model, terrain_mesh);
create_fallback_normal_texture(terrain_model, terrain_mesh);
create_fallback_specular_texture(terrain_model, terrain_mesh);

terrain_model.meshes.push_back(terrain_mesh);
upload_model_to_gpu(terrain_model, material_ds_layout, material_ds_binding);
g_engine->models.push_back(terrain_model);
g_engine->entities.push_back(create_entity(g_engine->entity_id++, 0, g_engine->models[0].name, glm::vec3(0.0f, 0.0f, 0.0f)));
#endif


#if 0
quad_tree = create_quad_tree({ 500.0f, 500.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f });
#endif
