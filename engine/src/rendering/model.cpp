#include "pch.h"
#include "model.h"

#include "vertex.h"
#include "api/vulkan/vk_image.h"
#include "filesystem/vfs.h"
#include "logging.h"

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

static void load_mesh_texture(Model& model, Mesh& mesh, const std::vector<std::filesystem::path>& paths)
{
    std::vector<std::filesystem::path>& uniques = model.unique_texture_paths;

    for (std::size_t i = 0; i < paths.size(); ++i) {
        uint32_t index = 0;
        const auto it = std::find(uniques.begin(), uniques.end(), paths[i]);
        
        if (it == uniques.end()) {
            vk_image texture = create_texture(paths[i].string());
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

static void create_fallback_mesh_texture(Model& model, 
    Mesh& mesh, 
    unsigned char* texture,
    const std::filesystem::path& path)
{

    std::vector<std::filesystem::path>& uniques = model.unique_texture_paths;

    uint32_t index = 0;
    const auto it = std::find(uniques.begin(), uniques.end(), path);

    if (it == uniques.end()) {
        vk_image image = create_texture(texture, 1, 1, VK_FORMAT_R8G8B8A8_SRGB);
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




static Mesh process_mesh(Model& model, const aiMesh* ai_mesh, const aiScene* scene)
{
    Mesh mesh{};

    mesh.name = ai_mesh->mName.C_Str();

    // walk through each of the mesh's vertices
    for(std::size_t i = 0; i < ai_mesh->mNumVertices; ++i) {
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
        if(ai_mesh->HasTextureCoords(0)) {
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
    for(std::size_t i = 0; i < ai_mesh->mNumFaces; ++i) {
        const aiFace& face = ai_mesh->mFaces[i];

        // retrieve all indices of the face and store them in the indices vector
        // TODO: Would it be possible to do a memcpy of all indices into the vector
        // instead of doing a loop?
        for(std::size_t j = 0; j < face.mNumIndices; ++j)
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
        const auto& specular_path = get_texture_full_path(material, aiTextureType_SPECULAR, model.path);


        load_mesh_texture(model, mesh, diffuse_path);
        load_mesh_texture(model, mesh, normal_path);
        load_mesh_texture(model, mesh, specular_path);


        // TEMP: If any texture was not found then use the fallback textures
        // TODO: Instead of loading them textures again, we should use the default
        // existing textures


        unsigned char defaultAlbedo[4];
        defaultAlbedo[0] = (unsigned char)255;
        defaultAlbedo[1] = (unsigned char)255;
        defaultAlbedo[2] = (unsigned char)255;
        defaultAlbedo[3] = (unsigned char)255;

        unsigned char defaultNormal[4];
        defaultNormal[0] = (unsigned char)128;
        defaultNormal[1] = (unsigned char)128;
        defaultNormal[2] = (unsigned char)255;
        defaultNormal[3] = (unsigned char)255;

        unsigned char defaultSpecular[4];
        defaultSpecular[0] = (unsigned char)0;
        defaultSpecular[1] = (unsigned char)0;
        defaultSpecular[2] = (unsigned char)0;
        defaultSpecular[3] = (unsigned char)255;

        if (diffuse_path.empty()) {
            create_fallback_mesh_texture(model, mesh, defaultAlbedo, "albedo_fallback");
            print_log("%s using fallback albedo texture.\n", model.name.c_str());
        }

        if (normal_path.empty()) {
            create_fallback_mesh_texture(model, mesh, defaultNormal, "normal_fallback");
            print_log("%s using fallback normal texture.\n", model.name.c_str());
        }

        if (specular_path.empty()) {
            create_fallback_mesh_texture(model, mesh, defaultSpecular, "specular_fallback");
            print_log("%s using fallback specular texture.\n", model.name.c_str());
        }
    }
 
    return mesh;
}

static void process_node(Model& model, aiNode* node, const aiScene* scene)
{
    // process the current nodes meshes if they exist
    for (std::size_t i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* assimp_mesh = scene->mMeshes[node->mMeshes[i]];
        Mesh mesh = process_mesh(model, assimp_mesh, scene);

        model.meshes.push_back(mesh);
    }

    // process any children nodes and do the same thing
    for (std::size_t i = 0; i < node->mNumChildren; ++i) {
        process_node(model, node->mChildren[i], scene);
    }
}

bool load_model(Model& model, const std::filesystem::path& path, bool flipUVs)
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

bool create_model(Model& model, const char* data, std::size_t len, bool flipUVs /*= true*/)
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

void destroy_model(Model& model)
{
    destroy_images(model.unique_textures);
    for (auto& mesh : model.meshes) {
        destroy_vertex_array(mesh.vertex_array);
    }
}

void upload_model_to_gpu(Model& model, VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> bindings)
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


