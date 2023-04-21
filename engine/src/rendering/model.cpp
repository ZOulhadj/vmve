#include "pch.h"
#include "model.h"

#include "vertex.h"
#include "api/vulkan/vk_image.h"
#include "filesystem/vfs.h"
#include "utils/logging.h"


namespace engine {

    static std::vector<std::filesystem::path> get_texture_full_path(const aiMaterial* material,
        aiTextureType type,
        const std::filesystem::path& model_path)
    {
        std::vector<std::filesystem::path> paths;

        for (std::size_t i = 0; i < material->GetTextureCount(type); ++i) {
            aiString ai_path;

            if (material->GetTexture(type, i, &ai_path) == aiReturn_FAILURE) {
                print_log("Failed to load texture: %s\n", ai_path.C_Str());
                return {};
            }

            // HACK: A work around to getting the full path. Should look into
            // a proper implementation.
            paths.push_back(model_path.parent_path().string() + "/" + ai_path.C_Str());

        }

        return paths;
    }

    static bool load_mesh_texture(Model_Old& model, Mesh_Old& mesh, const std::vector<std::filesystem::path>& paths)
    {
        std::vector<std::filesystem::path>& uniques = model.unique_texture_paths;

        for (std::size_t i = 0; i < paths.size(); ++i) {
            uint32_t index = 0;
            const auto it = std::find(uniques.begin(), uniques.end(), paths[i]);

            if (it == uniques.end()) {
                std::optional<Vk_Image> texture = create_texture(paths[i].string());

                // TODO: Should return nullptr instead of object
                if (!texture.has_value())
                    return false;


                model.unique_textures.push_back(texture.value());
                uniques.push_back(paths[i]);
                index = model.unique_textures.size() - 1;
            }
            else {
                index = std::distance(model.unique_texture_paths.begin(), it);
            }

            // Find the index position of the current texture and
            // add it to the list of textures for the mesh 
            mesh.textures.push_back(index);
        }

        return true;
    }

    static void create_fallback_mesh_texture(Model_Old& model,
        Mesh_Old& mesh,
        unsigned char* texture,
        const std::filesystem::path& path)
    {

        std::vector<std::filesystem::path>& uniques = model.unique_texture_paths;

        uint32_t index = 0;
        const auto it = std::find(uniques.begin(), uniques.end(), path);

        if (it == uniques.end()) {
            Vk_Image image = create_texture(texture, 1, 1, VK_FORMAT_R8G8B8A8_SRGB);
            model.unique_textures.push_back(image);
            uniques.push_back(path);
            index = model.unique_textures.size() - 1;
        }
        else {
            index = std::distance(model.unique_texture_paths.begin(), it);
        }

        // Find the index position of the current texture and
        // add it to the list of textures for the mesh 
        mesh.textures.push_back(index);

    }

    void create_fallback_albedo_texture(Model_Old& model, Mesh_Old& mesh)
    {
        unsigned char defaultAlbedo[4];
        defaultAlbedo[0] = (unsigned char)255;
        defaultAlbedo[1] = (unsigned char)255;
        defaultAlbedo[2] = (unsigned char)255;
        defaultAlbedo[3] = (unsigned char)255;

        create_fallback_mesh_texture(model, mesh, defaultAlbedo, "albedo_fallback");
    }

    void create_fallback_normal_texture(Model_Old& model, Mesh_Old& mesh)
    {
        unsigned char defaultNormal[4];
        defaultNormal[0] = (unsigned char)128;
        defaultNormal[1] = (unsigned char)128;
        defaultNormal[2] = (unsigned char)255;
        defaultNormal[3] = (unsigned char)255;

        create_fallback_mesh_texture(model, mesh, defaultNormal, "albedo_normal");
    }

    void create_fallback_specular_texture(Model_Old& model, Mesh_Old& mesh)
    {
        unsigned char defaultSpecular[4];
        defaultSpecular[0] = (unsigned char)0;
        defaultSpecular[1] = (unsigned char)0;
        defaultSpecular[2] = (unsigned char)0;
        defaultSpecular[3] = (unsigned char)255;

        create_fallback_mesh_texture(model, mesh, defaultSpecular, "albedo_specular");
    }


    static Mesh_Old process_mesh(Model_Old& model, const aiMesh* ai_mesh, const aiScene* scene)
    {
        Mesh_Old mesh{};

        mesh.name = ai_mesh->mName.C_Str();

        // walk through each of the mesh's vertices
        for (std::size_t i = 0; i < ai_mesh->mNumVertices; ++i) {
            vertex vertex{};
            vertex.position = glm::vec3(ai_mesh->mVertices[i].x,
                ai_mesh->mVertices[i].y,
                ai_mesh->mVertices[i].z);

            if (ai_mesh->HasNormals()) {
                vertex.normal = glm::vec3(ai_mesh->mNormals[i].x,
                    ai_mesh->mNormals[i].y,
                    ai_mesh->mNormals[i].z);
            }


            if (ai_mesh->HasTangentsAndBitangents()) {
                vertex.tangent = glm::vec3(ai_mesh->mTangents[i].x,
                    ai_mesh->mTangents[i].y,
                    ai_mesh->mTangents[i].z);

                // NOTE: Not getting bi tangents since we can calculate them by
                // simply doing the cross product of the tangent and normal
                // TODO: Should be load it? This will make it so that we don't
                // need to perform any sort of cross product in the first place.
                //vertex.biTangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);

            }


            // check if texture coordinates exist
            if (ai_mesh->HasTextureCoords(0)) {
                // todo: "1.0f -" is used here. Double check that this is correct.

                // a vertex can contain up to 8 different texture coordinates.
                // We make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates,
                // so we always take the first set (0).
                vertex.uv = glm::vec2(ai_mesh->mTextureCoords[0][i].x,
                    ai_mesh->mTextureCoords[0][i].y);
            }

            mesh.vertices.push_back(vertex);
        }

        // now walk through each of the mesh's faces (a face is a mesh its triangle)
        // and retrieve the corresponding vertex indices.
        for (std::size_t i = 0; i < ai_mesh->mNumFaces; ++i) {
            const aiFace& face = ai_mesh->mFaces[i];

            // retrieve all indices of the face and store them in the indices vector
            // TODO: Would it be possible to do a memcpy of all indices into the vector
            // instead of doing a loop?
            for (std::size_t j = 0; j < face.mNumIndices; ++j)
                mesh.indices.push_back(face.mIndices[j]);
        }


        // process material for current mesh if it has any
        if (ai_mesh->mMaterialIndex >= 0) {
            const aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];


            // todo(zak): rewrite this so that we check if texture can be loaded and 
            // if so load, else revert to fallback.

            // Check if the any textures have already been loaded
            // Note that a material may have multiple textures of the same type, hence the vector
            // TODO: Find out when there might be multiple textures of the same type.
            const auto& diffuse_path = get_texture_full_path(material, aiTextureType_DIFFUSE, model.path);
            const auto& normal_path = get_texture_full_path(material, aiTextureType_DISPLACEMENT, model.path);
            const auto& specular_path = get_texture_full_path(material, aiTextureType_METALNESS, model.path);



            // TODO: This whole section needs to be rewritten asap.

            if (diffuse_path.empty() || !load_mesh_texture(model, mesh, diffuse_path)) {
                create_fallback_albedo_texture(model, mesh);
                print_log("%s using fallback albedo texture.\n", model.name.c_str());
            }

            if (normal_path.empty() || !load_mesh_texture(model, mesh, normal_path)) {
                create_fallback_normal_texture(model, mesh);
                print_log("%s using fallback normal texture.\n", model.name.c_str());
            }

            if (specular_path.empty() || !load_mesh_texture(model, mesh, specular_path)) {
                create_fallback_specular_texture(model, mesh);
                print_log("%s using fallback specular texture.\n", model.name.c_str());
            }
        }

        return mesh;
    }

    static void process_node(Model_Old& model, aiNode* node, const aiScene* scene)
    {
        // process the current nodes meshes if they exist
        for (std::size_t i = 0; i < node->mNumMeshes; ++i) {
            const aiMesh* assimp_mesh = scene->mMeshes[node->mMeshes[i]];
            Mesh_Old mesh = process_mesh(model, assimp_mesh, scene);

            model.meshes.push_back(mesh);
        }

        // process any children nodes and do the same thing
        for (std::size_t i = 0; i < node->mNumChildren; ++i) {
            process_node(model, node->mChildren[i], scene);
        }
    }

    bool load_model(Model_Old& model, const std::filesystem::path& path, bool flipUVs)
    {
        print_log("Loading mesh %s\n", path.string().c_str());

        Assimp::Importer importer;

        unsigned int flags = aiProcessPreset_TargetRealtime_Fast |
            aiProcess_FlipWindingOrder |
            aiProcess_MakeLeftHanded |
            aiProcess_GenBoundingBoxes |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph |
            aiProcess_ImproveCacheLocality;

        if (flipUVs)
            flags |= aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(path.string(), flags);

        if (!scene) {
            return false;
        }


        // TEMP: Set model original path so that textures know where
        // they should load the files from
        model.path = path.string();
        model.name = path.filename().string();

        // Start processing from the root scene node
        process_node(model, scene->mRootNode, scene);

        print_log("Successfully loaded model with %llu meshes at path %s\n", model.meshes.size(), path.string().c_str());

        return true;
    }

    bool create_model(Model_Old& model, const char* data, std::size_t len, bool flipUVs /*= true*/)
    {
        print_log("Creating mesh\n");

        Assimp::Importer importer;

        unsigned int flags = aiProcessPreset_TargetRealtime_Fast |
            aiProcess_FlipWindingOrder |
            aiProcess_MakeLeftHanded |
            aiProcess_GenBoundingBoxes |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph |
            aiProcess_ImproveCacheLocality;

        if (flipUVs)
            flags |= aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFileFromMemory(data, len, flags);

        if (!scene) {
            return false;
        }


        // TEMP: Set model original path so that textures know where
        // they should load the files from
        model.path = "";
        model.name = std::string(scene->mName.C_Str());

        // Start processing from the root scene node
        process_node(model, scene->mRootNode, scene);

        print_log("Successfully created model from memory\n");

        return true;
    }

    void destroy_model(Model_Old& model)
    {
        destroy_images(model.unique_textures);
        for (auto& mesh : model.meshes) {
            destroy_vertex_array(mesh.vertex_array);
        }
    }

    void upload_model_to_gpu(Model_Old& model, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> bindings)
    {

        // At this point, the model has been fully loaded onto the CPU and now we 
        // need to transfer this data onto the GPU.
        // 
        // Also setup the texture descriptor sets

        for (auto& mesh : model.meshes) {
            mesh.vertex_array = create_vertex_array(mesh.vertices, mesh.indices);
            mesh.descriptor_set = allocate_descriptor_set(layout);

            for (std::size_t j = 0; j < mesh.textures.size(); ++j) {
                //assert(mesh.textures.size() == 3);

                update_binding(mesh.descriptor_set,
                    bindings[j],
                    model.unique_textures[mesh.textures[j]],
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    model.unique_textures[mesh.textures[j]].sampler);
            }
        }
    }



    model_mesh::model_mesh(const std::string& name)
        : m_name(name)
    {

    }

    void model_mesh::set_indices(const std::vector<uint32_t>& indices)
    {
        m_indices = std::move(indices);
    }

    model::model()
    {

    }

    model::~model()
    {

    }

    bool model::create(const std::filesystem::path& path)
    {
        const std::optional<tinygltf::Model> gltf_model = load_model(path);
        if (!gltf_model)
            return false;

        load_materials(gltf_model.value());
        load_textures(gltf_model.value());

        // get vertex/index buffer sizes upfront

        const tinygltf::Scene& scene = gltf_model->scenes[gltf_model->defaultScene];
        for (const int i : scene.nodes)
            traverse_node(gltf_model.value(), gltf_model->nodes[i]);


        return true;
    }

    std::optional<tinygltf::Model> model::load_model(const std::filesystem::path& path)
    {
        tinygltf::TinyGLTF loader;

        tinygltf::Model model;
        std::string warn, err;
        bool loaded = false;

        const std::filesystem::path& extension = path.extension();

        if (extension == ".gltf")
            loaded = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
        else if (extension == ".glb")
            loaded = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
        else {
            assert((extension == ".gltf" || extension == ".glb") && "glTF is the only currently supported model format");
            print_error("glTF is the only currently supported model format\n");

            return std::nullopt;
        }

        if (!warn.empty())
            print_warning("%s\n", warn.c_str());

        if (!loaded) {
            if (!err.empty())
                print_error("%s\n", err.c_str());

            return std::nullopt;
        }

        return model;
    }


    void model::load_materials(const tinygltf::Model& gltf_model)
    {

    }


    void model::load_textures(const tinygltf::Model& gltf_model)
    {

    }


    void model::traverse_node(const tinygltf::Model& gltf_model, const tinygltf::Node& gltf_node)
    {
        parse_node(gltf_model, gltf_node);

        for (int node : gltf_node.children)
            traverse_node(gltf_model, gltf_model.nodes[node]);
    }

    void model::parse_node(const tinygltf::Model& gltf_model, const tinygltf::Node& gltf_node)
    {
        // todo(zak): figure out which node type this is such as mesh/camera/light
        // currently, only meshes are handled


        if (gltf_node.mesh <= -1) {
            // todo(zak): was not a mesh not and could have been a camera/light node.
            // This needs to be handled
            return;
        }

        parse_mesh(gltf_model, gltf_model.meshes[gltf_node.mesh]);
    }

    void model::parse_mesh(const tinygltf::Model& gltf_model, const tinygltf::Mesh& gltf_mesh)
    {
        model_mesh mesh(gltf_mesh.name);

        // preallocate buffers

#if 0


        for (std::size_t i = 0; i < pos->count; ++i) {
        }
            Vertex v{};
            v.position = glm::make_vec3(&pos->data[i * pos->element_count]);
            v.normal = nor ? glm::make_vec3(&nor->data[i * nor->element_count]) : glm::vec3(0.0f);
            v.uv = tex ? glm::make_vec2(&tex->data[i * tex->element_count]) : glm::vec2(0.0f);
            v.color = col ? glm::make_vec3(&col->data[i * col->element_count]) : glm::vec3(0.0f);
            node.vertices.push_back(v);


        // handle rest of the attributes
        for (const tinygltf::Primitive& gltf_primitive : gltf_mesh.primitives) {
            mesh_primitive primitive;
            primitive.m_draw_mode = gltf_primitive.mode;

            const auto positions = get_attribute_data(gltf_model, gltf_primitive, "POSITION");
            const auto normals = get_attribute_data(gltf_model, gltf_primitive, "NORMAL");
            const auto tex_coord_0 = get_attribute_data(gltf_model, gltf_primitive, "TEXCOORD_0");

            if (gltf_primitive.indices != -1) {
                const tinygltf::Accessor& accessor = gltf_model.accessors[gltf_primitive.indices];
                const tinygltf::BufferView& buffer_view = gltf_model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = gltf_model.buffers[buffer_view.buffer];

                //node.indices.resize(accessor.count);
                const void* data = &(buffer.data[accessor.byteOffset + buffer_view.byteOffset]);
                const auto index_type = accessor.componentType;
                if (index_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const auto buf = static_cast<const uint32_t*>(data);
                    mesh.set_indices(std::vector<uint32_t>(buf, buf + accessor.count));
                } else if (index_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const auto buf = static_cast<const uint16_t*>(data);
                    mesh.set_indices(std::vector<uint32_t>(buf, buf + accessor.count));
                } else if (index_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const auto buf = static_cast<const uint8_t*>(data);
                    mesh.set_indices(std::vector<uint32_t>(buf, buf + accessor.count));
                }
              

            }

            // check if it has a material
            if (gltf_primitive.material != -1) {
                primitive.m_material_index = gltf_primitive.material;
            }
        }
#endif
    }

    std::optional<attribute_data> model::get_attribute_data(const tinygltf::Model& gltf_model, const tinygltf::Primitive& primitive, std::string_view attribute_name)
    {
        const auto iter = primitive.attributes.find(attribute_name.data());
        if (iter == primitive.attributes.end())
            return std::nullopt;

        const tinygltf::Accessor& accessor = gltf_model.accessors[iter->second];
        const tinygltf::BufferView& view = gltf_model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = gltf_model.buffers[view.buffer];


        attribute_data data{};
        data.data = reinterpret_cast<const float*>(&(buffer.data[accessor.byteOffset + view.byteOffset]));
        data.element_count = accessor.ByteStride(view) / sizeof(float);
        data.count = accessor.count;

        return data;
    }

}



