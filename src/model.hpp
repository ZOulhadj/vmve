#ifndef MY_ENGINE_MODEL_HPP
#define MY_ENGINE_MODEL_HPP


#include "renderer/vertex_array.hpp"
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
    std::vector<image_buffer_t> textures;


    VkDescriptorSet descriptor_set;
};


struct model_t {
    std::string path;
    std::vector<mesh_t> meshes;



    std::string name;

    vertex_array_t data;
    material_t textures;
};

vertex_array_t load_model(const std::string& path);
model_t load_model_new(const std::string& path);



model_t load_model_latest(const std::filesystem::path& path);


void destroy_model(model_t& model);

#endif
