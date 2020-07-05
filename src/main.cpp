#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <optional>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "util.h"
#include "Shader.h"
#include "errors.h"
#include "Window.h"
#include "OpenGLContext.h"
#include "pipeline.h"
#include "commands.h"
#include "Camera.h"
#include "texturing.h"

using namespace std;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texCoord;
};

struct InstanceAttrs {
    glm::mat4 mvp;
};

struct Uniforms {
    glm::mat4 viewProj;
};

struct ModelInfo {
    shared_ptr<Buffer> vertices;
    IndexBufferRef indices;
};

optional<ModelInfo> import_model(const std::string& filepath, OpenGLContext& context) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filepath,
//        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
//        aiProcess_GenNormals |
//        aiProcess_OptimizeMeshes |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if(!scene) {
        cout << importer.GetErrorString() << endl;
        return nullopt;
    }

    for(int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *material = scene->mMaterials[i];
        cout << "found material: " << material->GetName().C_Str() << endl;
        for(int j = 0; j < material->mNumProperties; j++) {
            aiMaterialProperty *prop = material->mProperties[j];
            aiString *key = &prop->mKey;
            if(prop->mType == aiPropertyTypeInfo::aiPTI_String) {
                cout << key->C_Str() << " = " << ((aiString *) prop->mData)->C_Str() << endl;
            } else {
                cout << key->C_Str() << " = " << prop->mType << " " << prop->mSemantic << endl;
            }

        }
    }

    int totalVertices = 0;
    int totalIndices = 0;

    for(int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[i];
        cout << "found mesh: " << mesh->mName.C_Str() << endl;
        cout << "vertices: " << mesh->mNumVertices << endl;

        totalVertices += mesh->mNumVertices;
        totalIndices += mesh->mNumFaces * 3;
    }


    auto indices = context.buildBuffer(BufferUsage::STATIC_DRAW, sizeof(uint32_t) * totalIndices, GL_MAP_WRITE_BIT);
    cout << "setting buffer storage for " << totalIndices << " indices " << endl;

    context.withMappedBuffer(*indices, 0, indices->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT, [scene](void *buf) -> void {
        uint32_t *indices = static_cast<uint32_t *>(buf);

        int idx = 0;
        for(int i = 0; i < scene->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[i];

            for(int k = 0; k < mesh->mNumFaces; k++) {
                aiFace face = mesh->mFaces[k];

                for(int j = 0; j < face.mNumIndices; j++) {
                    int index = face.mIndices[j];

                    indices[idx++] = index;
                }
            }
        }
    });

    auto vertices = context.buildBuffer(BufferUsage::STATIC_DRAW, sizeof(Vertex) * totalVertices, GL_MAP_WRITE_BIT);
    cout << "setting buffer storage for " << totalVertices << " vertices " << endl;

    context.withMappedBuffer(*vertices, 0, vertices->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT, [scene](void *buf) -> void  {
        Vertex *positions = static_cast<Vertex *>(buf);

        int idx = 0;
        for(int i = 0; i < scene->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[i];

            for(int k = 0; k < mesh->mNumVertices; k++) {
                aiVector3D vertex = mesh->mVertices[k];

                aiVector3D normal;
                if(mesh->mNormals != nullptr) {
                    normal = mesh->mNormals[k];
                }
                aiVector3D textureCoordinate = mesh->mTextureCoords[0][k];

                positions[idx].pos = glm::vec3(vertex.x, vertex.y, vertex.z);
                positions[idx].texCoord = glm::vec2(textureCoordinate.x, textureCoordinate.y);
                positions[idx++].norm = glm::vec3(normal.x, normal.y, normal.z);
            }
        }
    });

    ModelInfo info = {
        .vertices = vertices,
        .indices = {
            .buffer = indices,
            .format = IndexFormat::UINT32
        }
    };

    return make_optional(info);
}

const char *VERTEX_SHADER = R""(
#version 420

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;
layout(location = 3) in mat4 modelMatrix;

layout(std140, binding = 0) uniform MatrixBlock {
  mat4 viewProjectionMatrix;
};

out vec2 texCoord;
out vec3 normal;

void main(void) {
    //vec3 offset = vec3(0.5f * (gl_InstanceID % 10), 0, 0.5f * (gl_InstanceID / 10));
	gl_Position = viewProjectionMatrix * modelMatrix * vec4(vertexPosition, 1.0);
    texCoord = vertexTexCoord;
    normal = vertexNormal;
}
)"";

const char *FRAGMENT_SHADER = R""(
#version 420

in vec3 normal;
in vec2 texCoord;
out vec4 color;

layout(binding = 0) uniform sampler2D diffuseTexture;

void main(void) {
	color = vec4(texture(diffuseTexture, texCoord.xy).rgb + normal.xyz, 0.5f);
}
)"";

const float MOUSE_SENSITIVITY = 1.0/1000.0;
const float MOVEMENT_SPEED = 0.05f;
const int NUM_BUNNIES = 100;

int main() {

    Window window(Dimensions2d(1024, 768), "GameEngineCpp");

    OpenGLContext context(window);
    //context.setSwapInterval(1);

    {
        auto vertexShader = Shader::build(ShaderType::VERTEX, "awesome new shader", VERTEX_SHADER);
        auto fragmentShader = Shader::build(ShaderType::FRAGMENT, "awesome new fragment shader", FRAGMENT_SHADER);
//    Program p = Program::build({ std::move(v), std::move(f) });

        GraphicsPipelineCreateInfo pipelineCreateInfo = {
            .shaders = {
                .vertex = vertexShader,
                .fragment = fragmentShader
            },
            .vertexInput = {
                .bindings = {
                    {.binding=0, .stride=sizeof(Vertex), .inputRate=InputRate::PER_VERTEX},
                    {.binding=1, .stride=sizeof(InstanceAttrs), .inputRate=InputRate::PER_INSTANCE}
                },
                .attributes = {
                    {.location=0, .binding=0, .format=DataFormat::R32G32B32_SFLOAT, .offset=offsetof(Vertex, pos) },
                    {.location=1, .binding=0, .format=DataFormat::R32G32B32_SFLOAT, .offset=offsetof(Vertex, norm) },
                    {.location=2, .binding=0, .format=DataFormat::R32G32_SFLOAT, .offset=offsetof(Vertex, texCoord) },
                    {.location=3, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=offsetof(InstanceAttrs, mvp) },
                    {.location=4, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=offsetof(InstanceAttrs, mvp) + 4*sizeof(GLfloat) },
                    {.location=5, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=offsetof(InstanceAttrs, mvp) + 8*sizeof(GLfloat) },
                    {.location=6, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=offsetof(InstanceAttrs, mvp) + 12*sizeof(GLfloat) }
                }
            },
            .inputAssembler {
                .topology = PrimitiveTopology::TRIANGLES,
                .primitiveRestartEnable = false
            },
            .rasterizer {
            },
            .depthStencil {
                .depthTest = make_optional(ComparisonFunction::LESS_THAN_OR_EQUAL_TO),
                .depthMask = true
            },
            .colorBlend {
            },
        };
        shared_ptr<GraphicsPipeline> pipeline = context.buildPipeline(pipelineCreateInfo);

        int frame = 0;

        window.setKeyCallback([&window](int key, int scancode, int action, int mods) {
            const char *name = glfwGetKeyName(key, scancode);
            if(action != GLFW_PRESS) {
                return;
            }
            if(name == nullptr) {
                cout << "no key name" << endl;
                return;
            }
            if(key == GLFW_KEY_F) {
                window.toggleFullscreen();
            }
            cout <<  "KEY: " << name << endl;
        });

        auto instanceAttrs = context.buildBuffer(BufferUsage::STREAM_DRAW, sizeof(InstanceAttrs) * NUM_BUNNIES, GL_MAP_WRITE_BIT);

        auto uniforms = context.buildBuffer(BufferUsage::DYNAMIC_DRAW, sizeof(Uniforms) * 1, GL_MAP_WRITE_BIT);

        Camera camera(window, MOUSE_SENSITIVITY, MOVEMENT_SPEED);

        int x,y,n;
        unsigned char *data = stbi_load("/home/chris/code/game_engine/res/textures/LSCM_bunny_texture.png", &x, &y, &n, 0);
        if(data == nullptr) {
            cout << stbi_failure_reason() << endl;
        }
        auto tex = context.buildTexture2D(DataFormat::R8G8B8_UINT, Dimensions2d(x, y), true);
        context.uploadBaseImage2D(*tex, TransferFormat::R8G8B8_UINT, data);

        cout << "loaded " << x << ", " << y << ", " << n << endl;

        auto linearFiltering = Sampler::build({
            .magFilter = SamplerFilter::LINEAR,
            .minFilter = SamplerFilter::LINEAR,
            .mipmapMode = SamplerMipmapMode::LINEAR,
            .addressModeU = SamplerAddressMode::CLAMP_TO_EDGE,
            .addressModeV = SamplerAddressMode::CLAMP_TO_EDGE,
            .addressModeW = SamplerAddressMode::CLAMP_TO_EDGE,
            .cubemapSeamless = false
        });

        auto model = import_model("/home/chris/code/game_engine/res/models/LSCM_bunny.obj", context).value();

        shared_ptr<Buffer> vertices = model.vertices;
        auto indices = model.indices;

//        glEnable(GL_BLEND);
//        glBlendEquation(GL_FUNC_ADD);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        double time = 0;

        glm::mat4 previousViewProjMatrix = camera.calculateProjectionMatrix() * camera.calculateViewMatrix();

        while(!window.shouldClose()) {

            camera.processInput();

            // run loop

            time += 1;

            context.withMappedBuffer(*instanceAttrs, 0, instanceAttrs->size,
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT, [&time](void *buf) -> void {
                InstanceAttrs* instances = static_cast<InstanceAttrs *>(buf);
                for(int i = 0; i < NUM_BUNNIES; i++) {
                    glm::mat4 modelMatrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::vec3((i % 10) * 2.0f, 0, i / 5.0f));
                    modelMatrix = glm::rotate(modelMatrix, (float) time / 20.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    instances[i].mvp = modelMatrix;
                }
            });

            glm::mat4 newViewProjMatrix = camera.calculateProjectionMatrix() * camera.calculateViewMatrix();
            if(newViewProjMatrix != previousViewProjMatrix) {
                context.withMappedBuffer(*uniforms, 0, uniforms->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT, [&newViewProjMatrix](void *buf) -> void {
                    Uniforms* uniforms = static_cast<Uniforms*>(buf);
                    uniforms->viewProj = newViewProjMatrix;
                });
                previousViewProjMatrix = newViewProjMatrix;
            }

            context.withRenderTarget(context.getDefaultRenderTarget(), [&](auto guard) {
                guard.clear(ClearCommand(ColorRGBA(0.0, 0.0f, 0.0, 1.0), 1.0f));
                guard.draw({
                         .pipeline = pipeline,
                         .vertexBuffers = {
                                 {0, {.buffer = vertices, .offset = 0}},
                                 {1, {.buffer = instanceAttrs, .offset = 0}}
                         },
                         .uniformBuffers = {
                                 {0, {.buffer = uniforms, .offset = 0, .size = sizeof(Uniforms)}}
                         },
                         .textures = {
                                 {0, {.texture = tex, .sampler = linearFiltering}}
                         },
                         .call = IndexedDrawCall(208353, 0, indices),
                         .instanceCount = NUM_BUNNIES,
                         .firstInstance = 0
                 });
            });

            window.swapBuffers();
            window.pollEvents();

            frame++;
        }
    }

    glfwTerminate();
    // std::cout << "Hello, World!" << std::endl;

    return 0;
}
