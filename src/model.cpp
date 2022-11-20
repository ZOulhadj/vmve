#include "model.hpp"

#include "vertex.hpp"


static void parse_mesh(std::vector<vertex_t>& vertices, std::vector<uint32_t>& indices,
                       const aiMesh* mesh, const aiScene* scene)
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
            vertex.uv        = glm::vec2(1.0f - mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);
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

#if 0
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN


    // 1. diffuse maps
    vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
#endif

}

static void parse_model(std::vector<vertex_t>& vertices, std::vector<uint32_t>& indices,
                        aiNode* node, const aiScene* scene)
{

    for (std::size_t i = 0; i < node->mNumMeshes; ++i)
        parse_mesh(vertices, indices, scene->mMeshes[node->mMeshes[i]], scene);

    for (std::size_t i = 0; i < node->mNumChildren; ++i)
        parse_model(vertices, indices, node->mChildren[i], scene);

}

vertex_array_t load_model(const std::string& path)
{
    vertex_array_t buffer{};

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate |
                                                   aiProcess_PreTransformVertices |
                                                   aiProcess_CalcTangentSpace
                                                   );

    if (!scene) {
        printf("Failed to load model at path: %s\n", path.c_str());
        return {};
    }

    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;

    parse_model(vertices, indices, scene->mRootNode, scene);

    buffer = create_vertex_array(vertices, indices);


    return buffer;
}