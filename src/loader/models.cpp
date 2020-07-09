
#include "models.h"

#include "../../gen/shaders/textured.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

optional<ModelInfo> import_model(const std::string &filepath, OpenGLContext &context) {
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(filepath,
//        aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            //        aiProcess_GenNormals |
            //        aiProcess_OptimizeMeshes |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

    if (!scene) {
        cout << importer.GetErrorString() << endl;
        return nullopt;
    }

    for (int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *material = scene->mMaterials[i];
        cout << "found material: " << material->GetName().C_Str() << endl;
        for (int j = 0; j < material->mNumProperties; j++) {
            aiMaterialProperty *prop = material->mProperties[j];
            aiString *key = &prop->mKey;
            if (prop->mType == aiPropertyTypeInfo::aiPTI_String) {
                cout << key->C_Str() << " = " << ((aiString *) prop->mData)->C_Str() << endl;
            } else {
                cout << key->C_Str() << " = " << prop->mType << " " << prop->mSemantic << endl;
            }

        }
    }

    int totalVertices = 0;
    int totalIndices = 0;

    for (int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[i];
        cout << "found mesh: " << mesh->mName.C_Str() << endl;
        cout << "vertices: " << mesh->mNumVertices << endl;

        totalVertices += mesh->mNumVertices;
        totalIndices += mesh->mNumFaces * 3;
    }


    auto indices = make_shared<UntypedBuffer>(
            context.buildBuffer(BufferUsage::STATIC_DRAW, sizeof(uint32_t) * totalIndices, GL_MAP_WRITE_BIT));
    cout << "setting buffer storage for " << totalIndices << " indices " << endl;

    context.withMappedBuffer(*indices, 0, indices->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
            [scene](void *buf) -> void {
                uint32_t *indices = static_cast<uint32_t *>(buf);

                int idx = 0;
                for (int i = 0; i < scene->mNumMeshes; i++) {
                    aiMesh *mesh = scene->mMeshes[i];

                    for (int k = 0; k < mesh->mNumFaces; k++) {
                        aiFace face = mesh->mFaces[k];

                        for (int j = 0; j < face.mNumIndices; j++) {
                            int index = face.mIndices[j];

                            indices[idx++] = index;
                        }
                    }
                }
            });

    auto vertices = context.buildBuffer(BufferUsage::STATIC_DRAW,
            sizeof(pipelines::textured::VertexInput) * totalVertices, GL_MAP_WRITE_BIT);
    cout << "setting buffer storage for " << totalVertices << " vertices " << endl;

    context.withMappedBuffer(vertices, 0, vertices.size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
            [scene](void *buf) -> void {
                pipelines::textured::VertexInput *positions = static_cast<pipelines::textured::VertexInput*>(buf);

                int idx = 0;
                for (int i = 0; i < scene->mNumMeshes; i++) {
                    aiMesh *mesh = scene->mMeshes[i];

                    for (int k = 0; k < mesh->mNumVertices; k++) {
                        aiVector3D vertex = mesh->mVertices[k];

                        aiVector3D normal;
                        if (mesh->mNormals != nullptr) {
                            normal = mesh->mNormals[k];
                        }
                        aiVector3D textureCoordinate = mesh->mTextureCoords[0][k];

                        positions[idx].position = glm::vec3(vertex.x, vertex.y, vertex.z);
                        positions[idx].texCoord = glm::vec2(textureCoordinate.x, textureCoordinate.y);
                        positions[idx++].normal = glm::vec3(normal.x, normal.y, normal.z);
                    }
                }
            });

    return make_optional<ModelInfo>({
            .vertices = std::move(vertices),
            .indices = {
                    .buffer = indices,
                    .format = IndexFormat::UINT32
            }
    });
}
