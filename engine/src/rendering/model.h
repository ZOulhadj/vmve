#ifndef MY_ENGINE_MODEL_H
#define MY_ENGINE_MODEL_H


#include "api/vulkan/vk_vertex_array.h"
#include "material.h"

// One material per mesh

namespace engine {
    struct Mesh_Old
    {
        std::string name;

        std::vector<vertex> vertices;
        std::vector<uint32_t> indices;

        // A list of indices so we know we textures this mesh uses
        std::vector<uint32_t> textures;

        vk_vertex_array vertex_array;
        VkDescriptorSet descriptor_set;
    };

    struct Model_Old
    {
        std::string path;

        // A list of all the unique textures
        std::vector<std::filesystem::path> unique_texture_paths;
        std::vector<Vk_Image> unique_textures;

        std::vector<Mesh_Old> meshes;
        std::string name;
    };

    class texture
    {
    private:
        const unsigned char* data;
    };

    class material
    {
    private:
        // textures:
        // albedo
        // normal
        // roughness
        // metallic
    };

    class mesh_primitive
    {
    public:
        std::uint32_t m_draw_mode; // TRIANGLES, LINES, LOOPS
        std::uint32_t m_material_index;
    };

    class model_mesh
    {
    public:
        model_mesh(const std::string& name = "Unknown");


        void set_vertices();
        void set_indices(const std::vector<uint32_t>& indices);
    private:
        std::string m_name;

        std::vector<vertex> m_vertices;
        std::vector<std::uint32_t> m_indices;

        std::vector<mesh_primitive> m_primitves; // actual draw calls
    };



    struct attribute_data
    {
        const float* data;
        std::uint32_t element_count;
        std::uint32_t count;
    };

    class model // can also be known as scene
    {
    public:
        model();
        ~model();

        
        bool create(const std::filesystem::path& path);
    private:
        std::optional<tinygltf::Model> load_model(const std::filesystem::path& path);
        void load_materials(const tinygltf::Model& gltf_model);
        void load_textures(const tinygltf::Model& gltf_model);

        void traverse_node(const tinygltf::Model& gltf_model, const tinygltf::Node& gltf_node);

    private:

        void parse_node(const tinygltf::Model& gltf_model, const tinygltf::Node& gltf_node);
        void parse_mesh(const tinygltf::Model& gltf_model, const tinygltf::Mesh& gltf_mesh);

        std::optional<attribute_data> get_attribute_data(const tinygltf::Model& gltf_model, const tinygltf::Primitive& primitive, std::string_view attribute_name);

        // todo(zak)
        void parse_camera();
        void parse_light();

    private:
        std::vector<model_mesh> m_meshes;
        std::vector<material> m_materials;
        std::vector<texture> m_textures;

    };




    bool load_model(Model_Old& model, const std::filesystem::path& path, bool flipUVs = true);
    bool create_model(Model_Old& model, const char* data, std::size_t len, bool flipUVs = true);
    void destroy_model(Model_Old& model);

    void upload_model_to_gpu(Model_Old& model, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> bindings);


    // temp
    void create_fallback_albedo_texture(Model_Old& model, Mesh_Old& mesh);
    void create_fallback_normal_texture(Model_Old& model, Mesh_Old& mesh);
    void create_fallback_specular_texture(Model_Old& model, Mesh_Old& mesh);
}


#endif
