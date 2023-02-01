#ifndef MY_ENGINE_MODEL_HPP
#define MY_ENGINE_MODEL_HPP


#include "api/vulkan/vertex_array.hpp"
#include "material.hpp"

// One material per mesh

struct Mesh {
    std::string name;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    
    // A list of indices so we know we textures this mesh uses
    std::vector<uint32_t> textures;

    Vertex_Array vertex_array;
    VkDescriptorSet descriptor_set;
};

struct Mesh_Texture {
    std::filesystem::path path;
    Image_Buffer texture;
};

struct Model {
    std::string path;

    // A list of all the unique textures
    std::vector<std::filesystem::path> unique_texture_paths;
    std::vector<Image_Buffer> unique_textures;

    //std::vector<mesh_texture_t> unique_textures;
    
    std::vector<Mesh> meshes;

    std::string name;
};

Model load_model(const std::filesystem::path& path, bool flipUVs = true);
void destroy_model(Model& model);


void upload_model_to_gpu(Model& model, VkDescriptorSetLayout layout,
    std::vector<VkDescriptorSetLayoutBinding> bindings, VkSampler sampler);
#endif
