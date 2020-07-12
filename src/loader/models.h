
#pragma once

#include "../graphics/buffer.h"
#include "../graphics/OpenGLContext.h"
#include "../../gen/shaders/textured.h"

#define LOGURU_WITH_STREAMS 1
#include <loguru/loguru.hpp>

#include <glm/glm.hpp>
#include <span>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct ModelBufferSlices {
    Slice vertices;
    Slice indices;
    optional<Slice> instances;
};

struct ModelVertex {
    const aiScene* scene;
    size_t meshIdx;
    size_t vertexIdx;

    glm::vec3 readPosition();
    glm::vec3 readNormal();
    glm::vec2 readTextureCoordinate();
    glm::vec3 readTangent();
};

class Model {
    Assimp::Importer importer;
    const aiScene* scene;
    size_t totalIndices;
    size_t totalVertices;

    IndexFormat getPreferredIndexFormat();

public:
    Model(const char* filepath);

    ~Model() {
        LOG_S(INFO) << "model destructor";
    }

    size_t getNumIndices();
    size_t getNumVertices();

    void writeIndices(span<uint32_t> thisBuffer);

    template<typename T, typename F>
    void writeVertices(span<T> buffer, F callback) {
        int idx = 0;
        for (size_t i = 0; i < scene->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[i];

            for (size_t k = 0; k < mesh->mNumVertices; k++) {
                ModelVertex v = {
                        .scene = scene,
                        .meshIdx = i,
                        .vertexIdx = k
                };

                callback(&buffer[idx++], v);
            }
        }

        LOG_S(INFO) << "wrote " << idx << " vertices to buffer";
    }
};
