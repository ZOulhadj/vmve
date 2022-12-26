#ifndef MY_ENGINE_MODEL_HPP
#define MY_ENGINE_MODEL_HPP


#include "rendering/vulkan/vertex_array.hpp"
#include "material.hpp"


// A model may consist of multiple meshes. An example can be seen below,
// 
// Model: Car
//      Mesh: Body
//      Mesh: Windows
//      Mesh: Wheels
//      Mesh: Trunk
//
// etc.
// 
// Therefore, when loading a model we must parse potentially multiple sub-meshes
// and combine everything into a model.



// One material per mesh

struct mesh_t {
    std::string name;

    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;

    
    // A list of indices so we know we textures this mesh uses
    std::vector<uint32_t> textures;

    vertex_array_t vertex_array;
    VkDescriptorSet descriptor_set;
};

struct mesh_texture_t {
    std::filesystem::path path;
    image_buffer_t texture;
};

struct model_t {
    std::string path;

    // A list of all the unique textures
    std::vector<std::filesystem::path> unique_texture_paths;
    std::vector<image_buffer_t> unique_textures;

    //std::vector<mesh_texture_t> unique_textures;
    
    std::vector<mesh_t> meshes;

    std::string name;
};

model_t load_model(const std::filesystem::path& path);
void destroy_model(model_t& model);


void upload_model_to_gpu(model_t& model, VkDescriptorSetLayout layout,
    std::vector<VkDescriptorSetLayoutBinding>& bindings, VkSampler sampler);


#endif
