#include "model.hpp"

#include "rendering/vertex.hpp"
#include "rendering/vulkan/texture.hpp"
#include "filesystem/vfs.hpp"
#include "logging.hpp"

static std::vector<std::filesystem::path> get_texture_full_path(const aiMaterial* material, 
                                                   aiTextureType type, 
                                                   const std::filesystem::path& model_path)
{
    std::vector<std::filesystem::path> paths;

    for (std::size_t i = 0; i < material->GetTextureCount(type); ++i) {
        aiString ai_path;

        if (material->GetTexture(type, i, &ai_path) == aiReturn_FAILURE) {
            logger::err("Failed to load texture: {}", ai_path.C_Str());
            return {};
        }

        // HACK: A work around to getting the full path. Should look into
        // a proper implementation.
        paths.push_back(model_path.parent_path().string() + "/" + ai_path.C_Str());

    }

    return paths;
}

static void load_mesh_texture(model_t& model, mesh_t& mesh, const std::vector<std::filesystem::path>& paths)
{
    std::vector<std::filesystem::path>& uniques = model.unique_texture_paths;

    for (std::size_t i = 0; i < paths.size(); ++i) {
        uint32_t index = 0;
        const auto it = std::find(uniques.begin(), uniques.end(), paths[i]);
        
        if (it == uniques.end()) {
            image_buffer_t texture = load_texture(paths[i].string());
            model.unique_textures.push_back(texture);
            uniques.push_back(paths[i]);
            index = model.unique_textures.size() - 1;
        } else {
            index = std::distance(model.unique_texture_paths.begin(), it);

        }
         // Find the index position of the current texture and
        // add it to the list of textures for the mesh 
        mesh.textures.push_back(index);
    }
}



static mesh_t process_mesh(model_t& model, const aiMesh* assimp_mesh, const aiScene* scene)
{
    mesh_t mesh{};


    mesh.name = assimp_mesh->mName.C_Str();

    // walk through each of the mesh's vertices
    for(std::size_t i = 0; i < assimp_mesh->mNumVertices; ++i) {
        vertex_t vertex{};
        vertex.position = glm::vec3(assimp_mesh->mVertices[i].x, 
                                    assimp_mesh->mVertices[i].y, 
                                    assimp_mesh->mVertices[i].z);

        if (assimp_mesh->HasNormals()) {
            vertex.normal = glm::vec3(assimp_mesh->mNormals[i].x,
                                      assimp_mesh->mNormals[i].y,
                                      assimp_mesh->mNormals[i].z);
        }


        if (assimp_mesh->HasTangentsAndBitangents()) {
            vertex.tangent = glm::vec3(assimp_mesh->mTangents[i].x, 
                                       assimp_mesh->mTangents[i].y,
                                       assimp_mesh->mTangents[i].z);

            // NOTE: Not getting bi tangents since we can calculate them by
            // simply doing the cross product of the tangent and normal
            // TODO: Should be load it? This will make it so that we don't
            // need to perform any sort of cross product in the first place.
            //vertex.biTangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);

        }


        // check if texture coordinates exist
        if(assimp_mesh->HasTextureCoords(0)) {
            // todo: "1.0f -" is used here. Double check that this is correct.

            // a vertex can contain up to 8 different texture coordinates.
            // We make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates,
            // so we always take the first set (0).
            vertex.uv = glm::vec2(assimp_mesh->mTextureCoords[0][i].x, 
                                  assimp_mesh->mTextureCoords[0][i].y);
        }

        mesh.vertices.push_back(vertex);
    }

    // now walk through each of the mesh's faces (a face is a mesh its triangle)
    // and retrieve the corresponding vertex indices.
    for(std::size_t i = 0; i < assimp_mesh->mNumFaces; ++i) {
        const aiFace& face = assimp_mesh->mFaces[i];

        // retrieve all indices of the face and store them in the indices vector
        // TODO: Would it be possible to do a memcpy of all indices into the vector
        // instead of doing a loop?
        for(std::size_t j = 0; j < face.mNumIndices; ++j)
            mesh.indices.push_back(face.mIndices[j]);
    }


    // process material for current mesh if it has any
    if (assimp_mesh->mMaterialIndex >= 0) {
        const aiMaterial* material = scene->mMaterials[assimp_mesh->mMaterialIndex];

        // Check if the any textures have already been loaded
        // Note that a material may have multiple textures of the same type, hence the vector
        // TODO: Find out when there might be multiple textures of the same type.
        const auto& diffuse_path = get_texture_full_path(material, aiTextureType_DIFFUSE, model.path);
        const auto& normal_path = get_texture_full_path(material, aiTextureType_DISPLACEMENT, model.path);
        const auto& specular_path = get_texture_full_path(material, aiTextureType_SPECULAR, model.path);


        load_mesh_texture(model, mesh, diffuse_path);
        load_mesh_texture(model, mesh, normal_path);
        load_mesh_texture(model, mesh, specular_path);


        // TEMP: If any texture was not found then use the fallback textures

        if (diffuse_path.empty()) {
            std::vector<std::filesystem::path> fallback_speculars = { get_vfs_path("/textures/null_specular.png") };
            load_mesh_texture(model, mesh, fallback_speculars);
        }

        if (normal_path.empty()) {
            std::vector<std::filesystem::path> fallback_normals = { get_vfs_path("/textures/null_normal.png") };
            load_mesh_texture(model, mesh, fallback_normals);
        }

        if (specular_path.empty()) {
            std::vector<std::filesystem::path> fallback_speculars = { get_vfs_path("/textures/null_specular.png") };
            load_mesh_texture(model, mesh, fallback_speculars);
        }
    }
 
    return mesh;
}

static void process_node(model_t& model, aiNode* node, const aiScene* scene)
{
    // process the current nodes meshes if they exist
    for (std::size_t i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* assimp_mesh = scene->mMeshes[node->mMeshes[i]];
        mesh_t mesh = process_mesh(model, assimp_mesh, scene);

        model.meshes.push_back(mesh);
    }

    // process any children nodes and do the same thing
    for (std::size_t i = 0; i < node->mNumChildren; ++i) {
        process_node(model, node->mChildren[i], scene);
    }
}


model_t load_model(const std::filesystem::path& path)
{
    logger::info("Loading model at path {}", path.string());

    model_t model{};

    Assimp::Importer importer;


    const unsigned int flags = aiProcessPreset_TargetRealtime_Fast |
                               aiProcess_FlipWindingOrder |
                               aiProcess_MakeLeftHanded |
                               aiProcess_FlipUVs
                               ;

    const aiScene* scene = importer.ReadFile(path.string(), flags);

    if (!scene) {
        logger::err("Failed to load model at path: {}", path.string());
        return {};
    }

   
    // TEMP: Set model original path so that textures know where
    // they should load the files from
    model.path = path.string();
    model.name = path.filename().string();

    // Start processing from the root scene node
    process_node(model, scene->mRootNode, scene);

    logger::info("Successfully loaded model containing {} meshes at path {}", model.meshes.size(), path.string());
    



    return model;
}

void destroy_model(model_t& model)
{
    destroy_images(model.unique_textures);
    for (auto& mesh : model.meshes) {
        destroy_vertex_array(mesh.vertex_array);
    }
}

void upload_model_to_gpu(model_t& model, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding>& bindings, VkSampler sampler)
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
                sampler);
        }
    }
}


