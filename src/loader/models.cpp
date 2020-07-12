
#include "models.h"

#include "../../gen/shaders/textured.h"



Model::Model(const char *filepath) {
    scene = importer.ReadFile(filepath,
            aiProcess_GenSmoothNormals            |
            aiProcess_CalcTangentSpace      |
            aiProcess_JoinIdenticalVertices |
            aiProcess_Triangulate           |
            aiProcess_GenUVCoords           |
            aiProcess_SortByPType
//                        aiProcess_OptimizeMeshes |
    );

    if (!scene) {
        string s = importer.GetErrorString();
        LOG_S(ERROR) << "Assimp Error: " << s;
        throw s;
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

    totalVertices = 0;
    totalIndices = 0;

    for (int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[i];
        cout << "found mesh: " << mesh->mName.C_Str() << endl;
        cout << "vertices: " << mesh->mNumVertices << endl;

        totalVertices += mesh->mNumVertices;
        totalIndices += mesh->mNumFaces * 3;
    }
}

IndexFormat Model::getPreferredIndexFormat() {
    if(totalVertices < SHRT_MAX) {
        return IndexFormat::UINT16;
    } else {
        return IndexFormat::UINT32;
    }
}

size_t Model::getNumIndices() {
    return totalIndices;
}

size_t Model::getNumVertices() {
    return totalVertices;
}

void Model::writeIndices(span<uint32_t> thisBuffer) {
    assert(thisBuffer.size() >= totalIndices);

    int idx = 0;
    for (int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[i];

        for (int k = 0; k < mesh->mNumFaces; k++) {
            aiFace face = mesh->mFaces[k];

            for (int j = 0; j < face.mNumIndices; j++) {
                int index = face.mIndices[j];

                thisBuffer[idx++] = index;
            }
        }
    }

    LOG_S(INFO) << "wrote " << idx << " indices to buffer";
}

glm::vec3 ModelVertex::readTangent() {
    aiVector3D tangent;
    if (scene->mMeshes[meshIdx]->mTangents != nullptr) {
        tangent = scene->mMeshes[meshIdx]->mTangents[vertexIdx];
    } else {
        LOG_S(ERROR) << "expected tangents, found none";
    }
    return glm::vec3(tangent.x, tangent.y, tangent.z);
}

glm::vec2 ModelVertex::readTextureCoordinate() {
    if(scene->mMeshes[meshIdx]->mNumUVComponents[0] < 1) {
        LOG_S(ERROR) << "expected tex coords, found none";
        return glm::vec2(0, 0);
    }
    aiVector3D textureCoordinate = scene->mMeshes[meshIdx]->mTextureCoords[0][vertexIdx];
    return glm::vec2(textureCoordinate.x, textureCoordinate.y);
}

glm::vec3 ModelVertex::readNormal() {
    aiVector3D normal;
    if (scene->mMeshes[meshIdx]->mNormals != nullptr) {
        normal = scene->mMeshes[meshIdx]->mNormals[vertexIdx];
    } else {
        LOG_S(ERROR) << "expected normals, found none";
    }
    return glm::vec3(normal.x, normal.y, normal.z);
}

glm::vec3 ModelVertex::readPosition() {
    aiVector3D vec = scene->mMeshes[meshIdx]->mVertices[vertexIdx];
    return glm::vec3(vec.x, vec.y, vec.z);
}
