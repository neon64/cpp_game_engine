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
#include "textured_pipeline.h"
#include "fullscreen_quad_pipeline.h"

using namespace std;

struct ModelInfo {
    Buffer vertices;
    IndexBufferRef indices;
};

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


    auto indices = make_shared<Buffer>(context.buildBuffer(BufferUsage::STATIC_DRAW, sizeof(uint32_t) * totalIndices, GL_MAP_WRITE_BIT));
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

    auto vertices = context.buildBuffer(BufferUsage::STATIC_DRAW, sizeof(textured_pipeline::Vertex) * totalVertices, GL_MAP_WRITE_BIT);
    cout << "setting buffer storage for " << totalVertices << " vertices " << endl;

    context.withMappedBuffer(vertices, 0, vertices.size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
                             [scene](void *buf) -> void {
                                 textured_pipeline::Vertex *positions = static_cast<textured_pipeline::Vertex *>(buf);

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

                                         positions[idx].pos = glm::vec3(vertex.x, vertex.y, vertex.z);
                                         positions[idx].texCoord = glm::vec2(textureCoordinate.x, textureCoordinate.y);
                                         positions[idx++].norm = glm::vec3(normal.x, normal.y, normal.z);
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

const float MOUSE_SENSITIVITY = 1.0 / 1000.0;
const float MOVEMENT_SPEED = 0.2f;
const int NUM_BUNNIES_ROWS = 7;
const int NUM_BUNNIES_COLUMNS = 7;

int main() {

    Window window(Dimensions2d(1024, 768), "GameEngineCpp");
    window.grabMouseCursor();

    OpenGLContext context(window);
    //context.setSwapInterval(1);

    {
//    Program p = Program::build({ std::move(v), std::move(f) });

        auto pipeline = context.buildPipeline(textured_pipeline::pipeline());

        auto quadPipeline = context.buildPipeline(fullscreen_quad::pipeline());

        int frame = 0;

        window.setKeyCallback([&window](int key, int scancode, int action, int mods) {
            const char *name = glfwGetKeyName(key, scancode);
            if (action != GLFW_PRESS) {
                return;
            }
            if(key == GLFW_KEY_F) {
                window.toggleFullscreen();
            }
            if(key == GLFW_KEY_ESCAPE) {
                window.releaseMouseCursor();
            }

            if(name == nullptr) {
                cout << "no key name" << endl;
            } else {
                cout << "KEY: " << name << endl;
            }
        });

        auto instanceAttrs = context.buildBuffer(BufferUsage::STREAM_DRAW, sizeof(textured_pipeline::InstanceAttrs) * NUM_BUNNIES_ROWS * NUM_BUNNIES_COLUMNS,
                                                 GL_MAP_WRITE_BIT);

        auto uniforms = context.buildBuffer(BufferUsage::DYNAMIC_DRAW, sizeof(textured_pipeline::Uniforms) * 1, GL_MAP_WRITE_BIT);

        Camera camera(window, MOUSE_SENSITIVITY, MOVEMENT_SPEED);

        int x, y, n;
        unsigned char *data = stbi_load("/home/chris/code/game_engine/res/textures/LSCM_bunny_texture.png", &x, &y, &n,
                                        0);
        if (data == nullptr) {
            cout << stbi_failure_reason() << endl;
        }
        auto tex = context.buildTexture2D(DataFormat::R8G8B8_UINT, Dimensions2d(x, y), true);
        context.uploadBaseImage2D(tex, TransferFormat::R8G8B8_UINT, data);

//        Renderbuffer depthStorage = context.buildRenderbuffer(window.getSize().reduceSize(0), RenderbufferInternalFormat::D32_SFLOAT);
//        unique_ptr<DepthAttachment> depth = make_unique<OwnedDepthRenderbufferAttachment>(std::move(depthStorage));

        auto depthTex = context.buildTexture2D(DataFormat::D32_SFLOAT, window.getSize().reduceSize(0), false);
        unique_ptr<DepthAttachment> depthTexAttachment = make_unique<OwnedDepthTextureAttachment>(std::move(depthTex), 0);

        auto colorTex = context.buildTexture2D(DataFormat::R8G8B8A8_SRGB, window.getSize().reduceSize(0), false);
        unique_ptr<ColorAttachment> colorTexAttachment = make_unique<OwnedColorTextureAttachment>(std::move(colorTex), 0);

        unordered_map<int, unique_ptr<ColorAttachment>> colors;
        colors[0] = std::move(colorTexAttachment);
        auto framebuffer = context.buildFramebuffer(std::move(colors), make_optional(std::move(depthTexAttachment)), nullopt);
        GLenum i = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &i);

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

        auto nearestFiltering = Sampler::build({
                  .magFilter = SamplerFilter::NEAREST,
                  .minFilter = SamplerFilter::NEAREST,
                  .mipmapMode = SamplerMipmapMode::DISABLED,
                  .addressModeU = SamplerAddressMode::CLAMP_TO_EDGE,
                  .addressModeV = SamplerAddressMode::CLAMP_TO_EDGE,
                  .addressModeW = SamplerAddressMode::CLAMP_TO_EDGE,
                  .cubemapSeamless = false
          });

        auto model = import_model("/home/chris/code/game_engine/res/models/LSCM_bunny.obj", context).value();

        glm::vec3 verts[6] = {
            glm::vec3(-1.0f, -1.0f, 0),
            glm::vec3(1.0f, -1.0f, 0),
            glm::vec3(1.0f, 1.0f, 0),
            glm::vec3(1.0f, 1.0f, 0),
            glm::vec3(-1.0f, 1.0f, 0),
            glm::vec3(-1.0f, -1.0f, 0)
        };
        auto fullscreenQuad = context.buildBuffer(BufferUsage::STATIC_DRAW, sizeof(verts), verts, 0);

        auto& vertices = model.vertices;
        auto indices = model.indices;

//        glEnable(GL_BLEND);
//        glBlendEquation(GL_FUNC_ADD);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        double time = 0;

        glm::mat4 previousViewProjMatrix = camera.calculateProjectionMatrix() * camera.calculateViewMatrix();

        double lastFrameTime = glfwGetTime();
        double seconds = 0;

        while (!window.shouldClose()) {

            double thisFrameTime = glfwGetTime();
            double delta = thisFrameTime - lastFrameTime;
            lastFrameTime = thisFrameTime;

            // cout << delta << endl;

            camera.processInput();

            // run loop

            time += delta;
            seconds += delta;

            glm::vec3 cameraPos = glm::vec3(floor(camera.getPosition().x), 0.0f, floor(camera.getPosition().z));

            context.withMappedBuffer(instanceAttrs, 0, instanceAttrs.size,
                                     GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT, [&time, &cameraPos](void *buf) -> void {
                        textured_pipeline::InstanceAttrs *instances = static_cast<textured_pipeline::InstanceAttrs *>(buf);
                        int f = 0;
                        for(int i = 0; i < NUM_BUNNIES_ROWS; i++) {
                            for(int j = 0; j < NUM_BUNNIES_COLUMNS; j++) {
                                glm::mat4 modelMatrix = glm::mat4(1.0f);
                                modelMatrix = glm::translate(modelMatrix, (glm::vec3(i - NUM_BUNNIES_ROWS / 2, 0, j - NUM_BUNNIES_COLUMNS / 2) + cameraPos));
                                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f));
                                modelMatrix = glm::rotate(modelMatrix, (float) time / 20.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                                instances[f++].mvp = modelMatrix;
                            }
                        }
                    });

            glm::mat4 newViewProjMatrix = camera.calculateProjectionMatrix() * camera.calculateViewMatrix();
            if (newViewProjMatrix != previousViewProjMatrix) {
                context.withMappedBuffer(uniforms, 0, uniforms.size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
                                         [&newViewProjMatrix](void *buf) -> void {
                                             textured_pipeline::Uniforms *uniforms = static_cast<textured_pipeline::Uniforms *>(buf);
                                             uniforms->viewProj = newViewProjMatrix;
                                         });
                previousViewProjMatrix = newViewProjMatrix;
            }

            context.withRenderTarget(framebuffer, [&](auto guard) {
                guard.clear(ClearCommand(ColorRGBA(0.0, 0.0f, 0.0, 1.0), 1.0f));
                guard.draw({
                       .pipeline = pipeline,
                       .vertexBuffers = {
                               {0, {.buffer = vertices, .offset = 0}},
                               {1, {.buffer = instanceAttrs, .offset = 0}}
                       },
                       .uniformBuffers = {
                               {0, uniforms.asUniformBuffer()}
                       },
                       .textures = {
                               {0, {.texture = tex, .sampler = linearFiltering}}
                       },
                       .call = IndexedDrawCall(208353, 0, indices),
                       .instanceCount = NUM_BUNNIES_COLUMNS * NUM_BUNNIES_ROWS,
                       .firstInstance = 0
                 });
            });

            context.withDefaultRenderTarget([&](auto guard) {
                guard.clear(ClearCommand(ColorRGBA(0.0, 0.1f, 0.0, 1.0), 1.0f));

                guard.draw({
                    .pipeline = quadPipeline,
                    .vertexBuffers = {
                        {0, {.buffer = fullscreenQuad, .offset = 0}}
                    },
                    .uniformBuffers = {},
                    .textures = {
                        {0, {.texture = dynamic_cast<const OwnedDepthTextureAttachment*>(framebuffer.getDepthAttachment().value())->getTexture(), .sampler = nearestFiltering}},
                        {1, {.texture = dynamic_cast<const OwnedColorTextureAttachment*>(framebuffer.getColorAttachment(0).value())->getTexture(), .sampler = nearestFiltering}}
                    },
                    .call = NonIndexedDrawCall(6),
                    .instanceCount = 1,
                    .firstInstance = 0
                });

                guard.blit(framebuffer, framebuffer.getRect(), Rect2d::fromOrigin(300, 200), GL_COLOR_BUFFER_BIT, SamplerFilter::NEAREST);
            });

            window.swapBuffers();
            window.pollEvents();

            frame++;

            if(seconds >= 1) {
                cerr << "FPS: " << (double) frame / seconds << endl;
                frame = 0;
                seconds = 0;
            }
        }
    }

    Window::terminate();

    return 0;
}
