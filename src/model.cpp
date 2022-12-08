#include "model.hpp"

#include "vertex.hpp"

#include "vfs.hpp"

static texture_buffer_t load_mesh_texture(const aiMaterial* material, aiTextureType type, std::string_view path)
{
    texture_buffer_t texture;

    for (std::size_t i = 0; i < material->GetTextureCount(type); ++i) {
        aiString ai_path;
        std::string ai_path_string;

        if (material->GetTexture(type, i, &ai_path) == aiReturn_FAILURE) {
            ai_path_string = ai_path.C_Str();
            printf("Failed to load texture %s\n", ai_path_string.c_str());

            return {};
        }

        ai_path_string = ai_path.C_Str();
        // HACK: A work around to getting the full path. Should look into
        // a proper implementation.
        std::filesystem::path parent_path(path);
        texture = load_texture(parent_path.parent_path().string() + "/" + ai_path_string);
        printf("");
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
            vertex.biTangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
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
    for (std::size_t i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* material = scene->mMaterials[i];

        model.textures.albedo = load_mesh_texture(material, aiTextureType_DIFFUSE, p);
        model.textures.normal = load_mesh_texture(material, aiTextureType_HEIGHT, p);
        model.textures.specular = load_mesh_texture(material, aiTextureType_SPECULAR, p);
    }

}

static model_t parse_model(aiNode* node, const aiScene* scene, const std::string& p)
{
    model_t model;

    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;


    for (std::size_t i = 0; i < node->mNumMeshes; ++i)
        parse_mesh(model, vertices, indices, scene->mMeshes[node->mMeshes[i]], scene, p);

    //for (std::size_t i = 0; i < node->mNumChildren; ++i)
    //    parse_model(data, node->mChildren[i], scene);


    assert(!vertices.empty() || !indices.empty());


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

    const aiScene* scene = importer.ReadFile(path, aiProcess_PreTransformVertices |
        aiProcessPreset_TargetRealtime_Fast |
        aiProcess_FlipWindingOrder |
        aiProcess_MakeLeftHanded
    );

    if (!scene) {
        printf("Failed to load model at path: %s\n", path.c_str());
        return {};
    }

    return parse_model(scene->mRootNode, scene, path);
}
