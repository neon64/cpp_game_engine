#include <iostream>

#include <glad/glad.h>
#include <optional>
#include <span>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "lighting.h"
#include "util.h"
#include "graphics/Shader.h"
#include "errors.h"
#include "transform.h"
#include "Window.h"
#include "graphics/OpenGLContext.h"
#include "graphics/pipeline.h"
#include "graphics/commands.h"
#include "Camera.h"
#include "graphics/texturing.h"
#include "loader/texture.h"
#include "loader/shaders.h"
#include "loader/models.h"

#include "../gen/shaders/lighting_test.h"
#include "../gen/shaders/fullscreen.h"
#include "../gen/shaders/textured.h"

using namespace std;

const char* BUNNY_IMAGE = "/home/chris/code/game_engine/res/textures/LSCM_bunny_texture.png";
const char* DIAMOND_BLOCK_IMAGE = "/home/chris/code/game_engine/res/textures/Metal_Pattern_004_basecolor.jpg";
const char* NORMAL_MAP_IMAGE = "/home/chris/code/game_engine/res/textures/Metal_Pattern_004_normal.jpg";
const float MOUSE_SENSITIVITY = 1.5 / 1000.0;
const float MOVEMENT_SPEED = 0.05f;
const int NUM_BUNNIES_ROWS = 3;
const int NUM_BUNNIES_COLUMNS = 3;

struct Uniforms {
//    char foo[64];
//    alignas(32) pipelines::textured::MatrixBlock texturedMatrices;
    alignas(32) pipelines::lighting_test::MatrixBlock matrixBlock;
    alignas(32) pipelines::lighting_test::LightingBlock lighting;
    alignas(32) pipelines::lighting_test::Material material;
};

static pipelines::fullscreen::VertexInput FULLSCREEN_QUAD_VERTICES[6] = {
        pipelines::fullscreen::VertexInput{.position = glm::vec3(-1.0f, -1.0f, 0)},
        pipelines::fullscreen::VertexInput{.position = glm::vec3(1.0f, -1.0f, 0)},
        pipelines::fullscreen::VertexInput{.position = glm::vec3(1.0f, 1.0f, 0)},
        pipelines::fullscreen::VertexInput{.position = glm::vec3(1.0f, 1.0f, 0)},
        pipelines::fullscreen::VertexInput{.position = glm::vec3(-1.0f, 1.0f, 0)},
        pipelines::fullscreen::VertexInput{.position = glm::vec3(-1.0f, -1.0f, 0)}
};

void fillTexturedVertices(pipelines::textured::VertexInput *vertex, ModelVertex source) {
    vertex->position = source.readPosition();
    vertex->texCoord = source.readTextureCoordinate();
    vertex->normal = source.readNormal();
}

void fillTexturedVertices2(pipelines::lighting_test::VertexInput *vertex, ModelVertex source) {
    vertex->position = source.readPosition();
    vertex->texCoord = source.readTextureCoordinate();
//    LOG_S(INFO) << "normal: " << glm::to_string(source.readNormal());
    vertex->normal = source.readNormal();
    vertex->tangent = source.readTangent();
}

class Game {
public:
    Window *window;
    OpenGLContext *context;
    ShaderCache *shaderCache;
    Texture2dCache *textureCache;
    int frames = 0;
    double time = 0;
    glm::mat4 previousViewProjMatrix = glm::mat4(0);
    Camera *camera;

    ArrayBuffer<uint32_t> *indices;
    ArrayBuffer<pipelines::lighting_test::VertexInput> *vertices;
    //ArrayBuffer<pipelines::lighting_test::VertexInput> *vertices2;
    ArrayBuffer<pipelines::lighting_test::InstanceInput> *instanceAttrs;
    //ArrayBuffer<pipelines::lighting_test::InstanceInput> *instanceAttrs2;
    Buffer<Uniforms> *uniforms;
    Framebuffer *framebuffer;

    pipelines::textured::Pipeline *texturedPipeline;
    pipelines::fullscreen::Pipeline *quadPipeline;
    pipelines::lighting_test::Pipeline *lightingPipeline;

    vector<Transform> bunnyTransforms;
    Transform cubeTransform;
    shared_ptr<Texture2d> tex;
    shared_ptr<Texture2d> diamondTexture;
    shared_ptr<Texture2d> bricksNormalMap;
    Texture2d *bricksNoNormalMap;

    Sampler *linearFiltering;
    Sampler *linearFilteringWrap;
    Sampler *nearestFiltering;

    ModelBufferSlices bunnySlices;
    ModelBufferSlices cubeSlices;

    bool useNormalMap = true;

    ArrayBuffer<pipelines::fullscreen::VertexInput> *fullscreenQuad;

    Game() {
        window = new Window(Dimensions2d(1920, 1080), "GameEngineCpp", true);

        context = new OpenGLContext(*window);

        int a;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &a);
        LOG_S(ERROR) << "GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT = " << a;

        shaderCache = new ShaderCache(*context);
        textureCache = new Texture2dCache(*context);

        window->grabMouseCursor();
//        window->enterFullscreen();

        texturedPipeline = context->buildPipeline(pipelines::textured::Create{
                .shaders = shaderCache,
                .rasterizer {
                        .polygonMode = PolygonMode::FILL
                },
                .depthStencil = DepthStencilState::LESS_THAN_OR_EQUAL_TO,
                .colorBlend {
                        .attachments = {{0, {.blending = Blending::DISABLED}}}
                },
        }).onHeap();

        quadPipeline = context->buildPipeline(pipelines::fullscreen::Create{
                .shaders = shaderCache,
                .colorBlend {
                        .attachments = {{0, {.blending = Blending::DISABLED}}}
                },
        }).onHeap();

        lightingPipeline = context->buildPipeline(pipelines::lighting_test::Create{
                .shaders = shaderCache,
                .rasterizer = {
                    .culling = make_optional(CullMode::BACK)
                },
                .depthStencil = DepthStencilState::LESS_THAN_OR_EQUAL_TO,
        }).onHeap();

        window->setMouseButtonCallback([this](int button, int action, int mods) {
            window->grabMouseCursor();
        });

        window->setKeyCallback([this](int key, int scancode, int action, int mods) {
            const char *name = glfwGetKeyName(key, scancode);
            if (action != GLFW_PRESS) {
                return;
            }
            if(key == GLFW_KEY_F) {
                if(!window->isFullscreen()) {
                    window->grabMouseCursor();
                }
                window->toggleFullscreen();
            }
//            if(key == GLFW_KEY_U) {
//                updateUniforms();
//            }
            if(key == GLFW_KEY_N) {
                useNormalMap = !useNormalMap;
            }
            if(key == GLFW_KEY_ESCAPE) {
                if(window->isFullscreen()) {
                    window->exitFullscreen();
                } else {
                    window->releaseMouseCursor();
                }
            }

            if(name == nullptr) {
                cout << "no key name" << endl;
            } else {
                cout << "KEY: " << name << endl;
            }
        });

        cubeTransform.setPosition(glm::vec3(0.0, 0.0, 2.0));

        instanceAttrs = context->buildWritableArrayBuffer<pipelines::lighting_test::InstanceInput>(BufferUsage::DYNAMIC_DRAW,
                NUM_BUNNIES_ROWS * NUM_BUNNIES_COLUMNS + 1).onHeap();
//        instanceAttrs2 = context->buildWritableArrayBuffer<pipelines::lighting_test::InstanceInput>(BufferUsage::STATIC_DRAW,
//                1).onHeap();

        uniforms = context->buildWritableBuffer<Uniforms>(BufferUsage::DYNAMIC_DRAW).onHeap();

        camera = new Camera(*window, MOUSE_SENSITIVITY, MOVEMENT_SPEED);

        tex = textureCache->get(Texture2dMetadata(BUNNY_IMAGE, DesiredTextureFormat::DONT_CARE));
        diamondTexture = textureCache->get(Texture2dMetadata(DIAMOND_BLOCK_IMAGE, DesiredTextureFormat::DONT_CARE)); // new Texture2d(create1By1Texture(*context, glm::vec3(0.7f, 0.2f, 0.0f)));
        bricksNormalMap = textureCache->get(Texture2dMetadata(NORMAL_MAP_IMAGE, DesiredTextureFormat::DONT_CARE));
        bricksNoNormalMap = new Texture2d(create1By1NormalMap(*context, glm::vec3(0, 0, 1)));

        auto depthTex = context->buildTexture2D(DataFormat::D24_UNORM, window->getSize().reduceSize(0), false);
        unique_ptr<DepthAttachment> depthTexAttachment = make_unique<OwnedDepthTextureAttachment<Texture2d>>(
                std::move(depthTex),
                uint32_t(0));

        auto colorTex = context->buildTexture2D(DataFormat::R8G8B8A8_SRGB, window->getSize().reduceSize(0), false);
        unique_ptr<ColorAttachment> colorTexAttachment = make_unique<OwnedColorTextureAttachment<Texture2d>>(
                std::move(colorTex),
                0);

        unordered_map<int, unique_ptr<ColorAttachment>> colors;
        colors[0] = std::move(colorTexAttachment);
        framebuffer = new Framebuffer(
                context->buildFramebuffer(std::move(colors), make_optional(std::move(depthTexAttachment)),
                        nullopt));
        GLenum i = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &i);

        linearFiltering = new Sampler(Sampler::build(SamplerCreateInfo::ALL_LINEAR.withAddressMode(SamplerAddressMode::CLAMP_TO_EDGE)));
        linearFilteringWrap = new Sampler(Sampler::build(SamplerCreateInfo::ALL_LINEAR
                .withAddressMode(SamplerAddressMode::REPEAT)
                .withAnisotropicFiltering(8.0f)));
        nearestFiltering = new Sampler(Sampler::build(SamplerCreateInfo::ALL_NEAREST));

        auto bunny = Model("/home/chris/code/game_engine/res/models/LSCM_bunny.obj");
        auto cube = Model("/home/chris/code/game_engine/res/models/cube2/mesh.obj");

        indices = context->buildWritableArrayBuffer<uint32_t>(BufferUsage::STATIC_DRAW, bunny.getNumIndices() + cube.getNumIndices()).onHeap();
        vertices = context->buildWritableArrayBuffer<pipelines::lighting_test::VertexInput>(BufferUsage::STATIC_DRAW,
                bunny.getNumVertices() + cube.getNumVertices()).onHeap();
//        vertices2 = context->buildWritableArrayBuffer<pipelines::lighting_test::VertexInput>(BufferUsage::STATIC_DRAW, cube.getNumVertices()).onHeap();

        context->withMappedBuffer(indices->getSlice(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
                [&bunny, &cube, this](auto indices) {
                    bunny.writeIndices(indices);
                    bunnySlices.indices.elementOffset = 0;
                    bunnySlices.indices.numElements = bunny.getNumIndices();
                    cube.writeIndices(span(indices.data() + bunny.getNumIndices(), indices.size() - bunny.getNumIndices()));
                    cubeSlices.indices.elementOffset = 0 + bunnySlices.indices.numElements;
                    cubeSlices.indices.numElements = cube.getNumIndices();
                });
        context->withMappedBuffer(vertices->getSlice(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
                [&bunny, &cube, this](auto vertices) {
                    bunny.writeVertices(vertices, fillTexturedVertices2);
                    bunnySlices.vertices.elementOffset = 0;
                    bunnySlices.vertices.numElements = bunny.getNumVertices();
                    cube.writeVertices(std::span(vertices.data() + bunny.getNumVertices(), vertices.size() - bunny.getNumVertices()), fillTexturedVertices2);
                    cubeSlices.vertices.elementOffset = 0 + bunny.getNumVertices();
                    cubeSlices.vertices.numElements = cube.getNumVertices();
                });
//        context->withMappedBuffer(vertices2->getSlice(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
//                [&cube, this](auto vertices) {
//
//        });



        fullscreenQuad = context->buildStaticArrayBuffer(BufferUsage::STATIC_DRAW, std::span(FULLSCREEN_QUAD_VERTICES)).onHeap();
    }

    ~Game() {
        delete linearFiltering;
        delete nearestFiltering;
        delete fullscreenQuad;
        delete vertices;
        delete linearFilteringWrap;
        delete indices;
        delete instanceAttrs;
        delete quadPipeline;
        delete texturedPipeline;
        delete lightingPipeline;
        delete shaderCache;
        delete textureCache;
        delete context;
        delete camera;
        delete window;
    }

    void updateUniforms() {
        PointLight light(glm::vec3(-2.5f, 2.5f, -0.5f), glm::vec3(1, 0.7f, 0.5), Attenuation {
                .constant = 0.01f,
                .linear = 0.0f,
                .exponent = 0.01f
        });
        light.setAmbientCoefficient(0.15f);

        glm::mat4 proj = camera->calculateProjectionMatrix();
        glm::mat4 view = camera->calculateViewMatrix();
//        glm::mat4 newViewProjMatrix = proj * view;
        //if (newViewProjMatrix != previousViewProjMatrix) {
        context->withMappedBuffer(uniforms->getView(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
            [&](Uniforms* uniforms) {
                //uniforms->texturedMatrices.viewProjectionMatrix = proj * view;
                uniforms->matrixBlock.viewMatrix = view;
                uniforms->matrixBlock.projectionMatrix = proj;
                uniforms->material.materialShininess = 64.0f;
                uniforms->material.materialSpecularColor = glm::vec3(1.0, 1.0, 1.0);
                // uniforms->material.materialColor = glm::vec4(0.5f, 0.2f, 0.2f, 1.0f);
                uniforms->matrixBlock.cameraPosition = camera->getPosition();
                light.set(&uniforms->lighting.allLights[0]);
            });
    }

    void onFrame(double delta) {
        camera->processInput();

        time += delta;

        updateUniforms();
//
//        glm::vec3 cameraPos = glm::vec3(floor(camera->getPosition().x), 0.0f, floor(camera->getPosition().z));

        context->withMappedBuffer(instanceAttrs->getSlice(),
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
            [this, &delta](auto instances) {
                int f = 0;
                for (int i = 0; i < NUM_BUNNIES_ROWS; i++) {
                    for (int j = 0; j < NUM_BUNNIES_COLUMNS; j++) {
                        Transform t;
                        t.setPosition(glm::vec3(i - NUM_BUNNIES_ROWS / 2, 0,
                                j - NUM_BUNNIES_COLUMNS / 2));
                        t.setScale(glm::vec3(0.2f));
                        t.setOrientation(glm::rotate(t.getOrientation(), (float) time / 20.0f,
                                glm::vec3(0.0f, 1.0f, 0.0f)));
                        instances[f].modelMatrix = t.getModelMatrix();
                        instances[f++].normalMatrix = t.getNormalMatrix();
                    }
                }

                instances[f].modelMatrix = cubeTransform.getModelMatrix();
                instances[f].normalMatrix = cubeTransform.getNormalMatrix();
            });

//        if(cubeTransform.isDirty()) {
//            context->withMappedBuffer(instanceAttrs2->getSlice(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT, [this](auto instances) {
//
//            });
//        }


//        previousViewProjMatrix = newViewProjMatrix;
        //}

        context->withRenderTarget(*framebuffer, [&](auto guard) {
            guard.clear(ClearCommand(ColorRGBA(0.0f, 0.0f, 0.0f, 1.0), 1.0f));

            guard.draw(pipelines::lighting_test::DrawCmd {
                    .pipeline = *lightingPipeline,
                    .vertexBindings = pipelines::lighting_test::VertexBindings {
                            .perVertex = vertices->getSlice(),
                            .perInstance = instanceAttrs->getSlice()
                    },
                    .resourceBindings = pipelines::lighting_test::ResourceBindings{
                            .matrixBlock = uniforms->getView().accessField(pipelines::lighting_test::MatrixBlock,
                                    matrixBlock),
                            .material = uniforms->getView().accessField(pipelines::lighting_test::Material, material),
                            .lightingBlock = uniforms->getView().accessField(pipelines::lighting_test::LightingBlock, lighting),
                            .materialTexture = tex->withSampler(*linearFilteringWrap),
                            .normalMap = /*useNormalMap ? bricksNormalMap->withSampler(*linearFilteringWrap) :*/ bricksNoNormalMap->withSampler(*linearFilteringWrap)
                    },
                    .call = IndexedDrawCall(indices->getSlice().subslice(bunnySlices.indices)),
                    .instanceCount = NUM_BUNNIES_COLUMNS * NUM_BUNNIES_ROWS,
                    .firstInstance = 0
            });

            guard.draw(pipelines::lighting_test::DrawCmd {
                    .pipeline = *lightingPipeline,
                    .vertexBindings = pipelines::lighting_test::VertexBindings {
                            .perVertex = vertices->getSlice(),
                            .perInstance = instanceAttrs->getSlice()
                    },
                    .resourceBindings = pipelines::lighting_test::ResourceBindings {
                            .matrixBlock = uniforms->getView().accessField(pipelines::lighting_test::MatrixBlock,
                                    matrixBlock),
                            .material = uniforms->getView().accessField(pipelines::lighting_test::Material, material),
                            .lightingBlock = uniforms->getView().accessField(pipelines::lighting_test::LightingBlock, lighting),
                            .materialTexture = diamondTexture->withSampler(*linearFilteringWrap),
                            .normalMap = useNormalMap ? bricksNormalMap->withSampler(*linearFilteringWrap) : bricksNoNormalMap->withSampler(*linearFilteringWrap)
                    },
                    .call = IndexedDrawCall(indices->getSlice().subslice(cubeSlices.indices), cubeSlices.vertices.elementOffset),
                    .firstInstance = NUM_BUNNIES_COLUMNS * NUM_BUNNIES_ROWS
            });
        });

        context->withDefaultRenderTarget([&](auto guard) {
            guard.clear(ClearCommand(ColorRGBA(0.1f, 0.1f, 0.1f, 1.0), 1.0f));

//            guard.draw(pipelines::fullscreen::DrawCmd{
//                    .pipeline = *quadPipeline,
//                    .vertexBindings =  {
//                            .perVertex = fullscreenQuad->getSlice()
//                    },
//                    .resourceBindings = {
//                            .tex = dynamic_cast<OwnedDepthTextureAttachment<Texture2d> *>(*framebuffer->getDepthAttachment())->sample(
//                                    *linearFiltering),
//                            .colorTex = dynamic_cast<OwnedColorTextureAttachment<Texture2d> *>(*framebuffer->getColorAttachment(
//                                    0))->sample(*linearFiltering)
//                    },
//                    .call = NonIndexedDrawCall(6),
//            });
//
            guard.blit(*framebuffer, framebuffer->getRect(), Rect2d::fromOrigin(window->getSize().width, window->getSize().height), GL_COLOR_BUFFER_BIT,
                    SamplerFilter::NEAREST);
        });

        frames++;
    }

    void onSecond(double seconds) {
        cerr << "FPS: " << (double) frames / seconds << endl;
        frames = 0;
    }

    void enterLoop() {
        double lastFrameTime = glfwGetTime();
        double seconds = 0;

        // run loop
        while (!window->shouldClose()) {
            double thisFrameTime = glfwGetTime();
            double delta = thisFrameTime - lastFrameTime;
            lastFrameTime = thisFrameTime;

            onFrame(delta);

            window->swapBuffers();
            window->pollEvents();

            seconds += delta;

            if (seconds >= 1) {
                onSecond(seconds);
                seconds = 0;
            }
        }
    }
};

int main() {

    {
        // so that destructor runs before Window::terminate()
        Game game;
        game.enterLoop();
    }

    Window::terminate();

    return 0;
}
