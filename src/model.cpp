#include "model.hpp"

#include "vertex.hpp"
#include "renderer/texture.hpp"
#include "vfs.hpp"
#include "logging.hpp"

static image_buffer_t load_mesh_texture(const aiMaterial* material, aiTextureType type, std::string_view path)
{
    image_buffer_t texture;

    for (std::size_t i = 0; i < material->GetTextureCount(type); ++i) {
        aiString ai_path;
        std::string ai_path_string;

        if (material->GetTexture(type, i, &ai_path) == aiReturn_FAILURE) {
            ai_path_string = ai_path.C_Str();
            logger::err("Failed to load texture: {}", ai_path_string);
            return {};
        }

        ai_path_string = ai_path.C_Str();
        // HACK: A work around to getting the full path. Should look into
        // a proper implementation.
        std::filesystem::path parent_path(path);
        texture = load_texture(parent_path.parent_path().string() + "/" + ai_path_string);
    }

    return texture;
}

static void parse_mesh(model_t& model, std::vector<vertex_t>& vertices, 
                                       std::vector<uint32_t>& indices,
                                       aiMesh* mesh, const aiScene* scene, const std::string& p)
{
    // walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        vertex_t vertex{};

        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        // check if texture coordinates exist
        if(mesh->HasTextureCoords(0)) {
            // todo: "1.0f -" is used here. Double check that this is correct.

            // a vertex can contain up to 8 different texture coordinates.
            // We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates,
            // so we always take the first set (0).
            vertex.uv        = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            vertex.tangent   = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            //vertex.biTangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        } else {
            vertex.uv = glm::vec2(0.0f);
        }


        vertices.push_back(vertex);
    }

    // now walk through each of the mesh's faces (a face is a mesh its triangle)
    // and retrieve the corresponding vertex indices.
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];

        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }


    // process materials
    for (std::size_t i = 1; i < scene->mNumMaterials; ++i) {
        const aiMaterial* material = scene->mMaterials[i];

        model.textures.textures.push_back(load_mesh_texture(material, aiTextureType_DIFFUSE, p));
        model.textures.textures.push_back(load_mesh_texture(material, aiTextureType_HEIGHT, p));
        model.textures.textures.push_back(load_mesh_texture(material, aiTextureType_SPECULAR, p));
    }

}

static model_t parse_model(aiNode* node, const aiScene* scene, const std::string& p)
{
    model_t model{};

    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;


    for (std::size_t i = 0; i < node->mNumMeshes; ++i)
        parse_mesh(model, vertices, indices, scene->mMeshes[node->mMeshes[i]], scene, p);

    //for (std::size_t i = 0; i < node->mNumChildren; ++i)
    //    parse_model(data, node->mChildren[i], scene);


    assert(!vertices.empty() || !indices.empty());


    // TODO: String is not constructing from const char*, why?
    std::string n = node->mName.C_Str();

    model.name = n;
    model.data = create_vertex_array(vertices, indices);

    return model;
}

vertex_array_t load_model(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_PreTransformVertices |
        aiProcessPreset_TargetRealtime_Fast |
        aiProcess_ConvertToLeftHanded
    );

    if (!scene) {
        printf("Failed to load model at path: %s\n", path.c_str());
        return {};
    }

    model_t data = parse_model(scene->mRootNode, scene, path);
    return data.data;
}

model_t load_model_new(const std::string& path)
{
    Assimp::Importer importer;


    const unsigned int flags = aiProcess_PreTransformVertices |
                               aiProcessPreset_TargetRealtime_Fast |
                               aiProcess_FlipWindingOrder |
                               aiProcess_MakeLeftHanded;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene) {
        logger::err("Failed to load model at path: {}", path);
        return {};
    }

    return parse_model(scene->mRootNode, scene, path);
}




static std::vector<image_buffer_t> load_mesh_textures(const aiMaterial* material, aiTextureType type, std::string_view path)
{
    std::vector<image_buffer_t> textures;

    for (std::size_t i = 0; i < material->GetTextureCount(type); ++i) {
        aiString ai_path;
        std::string ai_path_string;

        if (material->GetTexture(type, i, &ai_path) == aiReturn_FAILURE) {
            ai_path_string = ai_path.C_Str();
            logger::err("Failed to load texture: {}", ai_path_string);
            return {};
        }

        ai_path_string = ai_path.C_Str();
        // HACK: A work around to getting the full path. Should look into
        // a proper implementation.
        std::filesystem::path parent_path(path);

        textures.push_back(load_texture(parent_path.parent_path().string() + "/" + ai_path_string));
    }

    return textures;
}




static mesh_t process_mesh(const model_t& model, const aiMesh* assimp_mesh, const aiScene* scene)
{
    mesh_t mesh{};


    mesh.name = assimp_mesh->mName.C_Str();

    // walk through each of the mesh's vertices
    for(unsigned int i = 0; i < assimp_mesh->mNumVertices; i++) {
        vertex_t vertex{};
        vertex.position = glm::vec3(assimp_mesh->mVertices[i].x, 
                                    assimp_mesh->mVertices[i].y, 
                                    assimp_mesh->mVertices[i].z);

        if (assimp_mesh->HasNormals()) {
            vertex.normal = glm::vec3(assimp_mesh->mNormals[i].x,
                                      assimp_mesh->mNormals[i].y,
                                      assimp_mesh->mNormals[i].z);
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

            vertex.tangent = glm::vec3(assimp_mesh->mTangents[i].x, 
                                       assimp_mesh->mTangents[i].y,
                                       assimp_mesh->mTangents[i].z);

            // NOTE: Not getting bi tangents since we can calculate them by
            // simply doing the cross product of the tangent and normal
            // TODO: Should be load it? This will make it so that we don't
            // need to perform any sort of cross product in the first place.
            //vertex.biTangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }

        mesh.vertices.push_back(vertex);
    }

    // now walk through each of the mesh's faces (a face is a mesh its triangle)
    // and retrieve the corresponding vertex indices.
    for(std::size_t i = 0; i < assimp_mesh->mNumFaces; ++i) {
        const aiFace& face = assimp_mesh->mFaces[i];

        // retrieve all indices of the face and store them in the indices vector
        for(std::size_t j = 0; j < face.mNumIndices; ++j)
            mesh.indices.push_back(face.mIndices[j]);
    }


    // process material for current mesh if it has any
    if (assimp_mesh->mMaterialIndex >= 0) {
        const aiMaterial* material = scene->mMaterials[assimp_mesh->mMaterialIndex];

        std::vector<image_buffer_t> diffuse = load_mesh_textures(material, aiTextureType_DIFFUSE, model.path);
        std::vector<image_buffer_t> normals = load_mesh_textures(material, aiTextureType_HEIGHT, model.path);
        std::vector<image_buffer_t> speculars = load_mesh_textures(material, aiTextureType_SPECULAR, model.path);


        mesh.textures.insert(mesh.textures.end(), diffuse.begin(), diffuse.end());
        mesh.textures.insert(mesh.textures.end(), normals.begin(), normals.end());
        mesh.textures.insert(mesh.textures.end(), speculars.begin(), speculars.end());
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


model_t load_model_latest(const std::filesystem::path& path)
{
    model_t model{};

    Assimp::Importer importer;


    const unsigned int flags = aiProcess_PreTransformVertices |
                               aiProcessPreset_TargetRealtime_Fast |
                               aiProcess_FlipWindingOrder |
                               aiProcess_MakeLeftHanded;

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


    // At this point, the model has been fully loaded onto the CPU and now we 
    // need to transfer this data onto the GPU.


    return model;
}

void destroy_model(model_t& model)
{
    // TODO: deallocate descriptor set

    for (auto& mesh : model.meshes)
        destroy_images(mesh.textures);


}


