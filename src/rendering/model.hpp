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

struct Mesh {
    std::string name;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    
    // A list of indices so we know we textures this mesh uses
    std::vector<uint32_t> textures;

    VertexArray vertex_array;
    VkDescriptorSet descriptor_set;
};

struct MeshTexture {
    std::filesystem::path path;
    ImageBuffer texture;
};

struct Model {
    std::string path;

    // A list of all the unique textures
    std::vector<std::filesystem::path> unique_texture_paths;
    std::vector<ImageBuffer> unique_textures;

    //std::vector<mesh_texture_t> unique_textures;
    
    std::vector<Mesh> meshes;

    std::string name;
};

Model LoadModel(const std::filesystem::path& path, bool flipUVs = true);
void DestroyModel(Model& model);


void UploadModelToGPU(Model& model, VkDescriptorSetLayout layout,
    std::vector<VkDescriptorSetLayoutBinding> bindings, VkSampler sampler);


#endif
