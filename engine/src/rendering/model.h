#ifndef MY_ENGINE_MODEL_H
#define MY_ENGINE_MODEL_H


#include "api/vulkan/vertex_array.h"
#include "material.h"

// One material per mesh

struct Mesh
{
    std::string name;

    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;
    
    // A list of indices so we know we textures this mesh uses
    std::vector<uint32_t> textures;

    vk_vertex_array vertex_array;
    VkDescriptorSet descriptor_set;
};

struct Model
{
    std::string path;

    // A list of all the unique textures
    std::vector<std::filesystem::path> unique_texture_paths;
    std::vector<vk_image> unique_textures;

    std::vector<Mesh> meshes;
    std::string name;
};

bool load_model(Model& model, const std::filesystem::path& path, bool flipUVs = true);
bool create_model(Model& model, const char* data, std::size_t len, bool flipUVs = true);
void destroy_model(Model& model);

void upload_model_to_gpu(Model& model, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> bindings);


#endif
