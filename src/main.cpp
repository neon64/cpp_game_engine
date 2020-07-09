#include <iostream>

#include <glad/glad.h>
#include <optional>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util.h"
#include "Shader.h"
#include "errors.h"
#include "Window.h"
#include "OpenGLContext.h"
#include "pipeline.h"
#include "commands.h"
#include "Camera.h"
#include "texturing.h"
#include "pipelines/quad.h"
#include "pipelines/textured.h"
#include "loader/texture.h"
#include "loader/models.h"

using namespace std;

const std::string BUNNY_IMAGE = "/home/chris/code/game_engine/res/textures/LSCM_bunny_texture.png";
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

        auto pipeline = context.buildPipeline(pipelines::textured::pipeline());

        auto quadPipeline = context.buildPipeline(pipelines::fullscreen::pipeline());

        int frame = 0;

        window.setKeyCallback([&window](int key, int scancode, int action, int mods) {
            const char *name = glfwGetKeyName(key, scancode);
            if (action != GLFW_PRESS) {
                return;
            }
            if (key == GLFW_KEY_F) {
                window.toggleFullscreen();
            }
            if (key == GLFW_KEY_ESCAPE) {
                window.releaseMouseCursor();
            }

            if (name == nullptr) {
                cout << "no key name" << endl;
            } else {
                cout << "KEY: " << name << endl;
            }
        });

        auto instanceAttrs = context.buildBuffer(BufferUsage::STREAM_DRAW,
                sizeof(pipelines::textured::InstanceInput) * NUM_BUNNIES_ROWS *
                NUM_BUNNIES_COLUMNS,
                GL_MAP_WRITE_BIT);

        auto uniforms = context.buildBuffer(BufferUsage::DYNAMIC_DRAW, sizeof(pipelines::textured::MatrixBlock) * 1,
                GL_MAP_WRITE_BIT);

        Camera camera(window, MOUSE_SENSITIVITY, MOVEMENT_SPEED);

        auto tex = loadTexture(context, BUNNY_IMAGE);

//        Renderbuffer depthStorage = context.buildRenderbuffer(window.getSize().reduceSize(0), RenderbufferInternalFormat::D32_SFLOAT);
//        unique_ptr<DepthAttachment> depth = make_unique<OwnedDepthRenderbufferAttachment>(std::move(depthStorage));

        auto depthTex = context.buildTexture2D(DataFormat::D24_UNORM, window.getSize().reduceSize(0), false);
        unique_ptr<DepthAttachment> depthTexAttachment = make_unique<OwnedDepthTextureAttachment>(std::move(depthTex),
                0);

        auto colorTex = context.buildTexture2D(DataFormat::R8G8B8A8_SRGB, window.getSize().reduceSize(0), false);
        unique_ptr<ColorAttachment> colorTexAttachment = make_unique<OwnedColorTextureAttachment>(std::move(colorTex),
                0);

        unordered_map<int, unique_ptr<ColorAttachment>> colors;
        colors[0] = std::move(colorTexAttachment);
        auto framebuffer = context.buildFramebuffer(std::move(colors), make_optional(std::move(depthTexAttachment)),
                nullopt);
        GLenum i = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &i);

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


        auto fullscreenQuad = context.buildBuffer(BufferUsage::STATIC_DRAW, sizeof(FULLSCREEN_QUAD_VERTICES), FULLSCREEN_QUAD_VERTICES, 0);

        auto &vertices = model.vertices;
        auto indices = model.indices;

        double time = 0;

        glm::mat4 previousViewProjMatrix = glm::mat4(0);

        double lastFrameTime = glfwGetTime();
        double seconds = 0;

        // run loop
        while (!window.shouldClose()) {

            double thisFrameTime = glfwGetTime();
            double delta = thisFrameTime - lastFrameTime;
            lastFrameTime = thisFrameTime;

            camera.processInput();

            time += delta;
            seconds += delta;

            glm::vec3 cameraPos = glm::vec3(floor(camera.getPosition().x), 0.0f, floor(camera.getPosition().z));

            context.withMappedBuffer(instanceAttrs, 0, instanceAttrs.size,
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
                    [&time, &cameraPos](void *buf) -> void {
                        pipelines::textured::InstanceInput *instances = static_cast<pipelines::textured::InstanceInput *>(buf);
                        int f = 0;
                        for (int i = 0; i < NUM_BUNNIES_ROWS; i++) {
                            for (int j = 0; j < NUM_BUNNIES_COLUMNS; j++) {
                                glm::mat4 modelMatrix = glm::mat4(1.0f);
                                modelMatrix = glm::translate(modelMatrix,
                                        (glm::vec3(i - NUM_BUNNIES_ROWS / 2, 0,
                                                j - NUM_BUNNIES_COLUMNS / 2) +
                                         cameraPos));
                                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f));
                                modelMatrix = glm::rotate(modelMatrix, (float) time / 20.0f,
                                        glm::vec3(0.0f, 1.0f, 0.0f));
                                instances[f++].modelMatrix = modelMatrix;
                            }
                        }
                    });

            glm::mat4 newViewProjMatrix = camera.calculateProjectionMatrix() * camera.calculateViewMatrix();
            if (newViewProjMatrix != previousViewProjMatrix) {
                context.withMappedBuffer(uniforms, 0, uniforms.size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
                        [&newViewProjMatrix](void *buf) -> void {
                            pipelines::textured::MatrixBlock *uniforms = static_cast<pipelines::textured::MatrixBlock *>(buf);
                            uniforms->viewProjectionMatrix = newViewProjMatrix;
                        });
                previousViewProjMatrix = newViewProjMatrix;
            }

            context.withRenderTarget(framebuffer, [&](auto guard) {
                guard.clear(ClearCommand(ColorRGBA(0.0, 0.0f, 0.0, 1.0), 1.0f));
                guard.draw(pipelines::textured::DrawCmd{
                        .pipeline = pipeline,
                        .vertexBindings = pipelines::textured::VertexBindings{
                                .perVertex = {.buffer = vertices, .offset = 0},
                                .perInstance = {.buffer = instanceAttrs, .offset = 0}
                        },
                        .resourceBindings = pipelines::textured::ResourceBindings{
                                .matrixBlock = {.buffer = uniforms, .offset = 0},
                                .diffuseTexture = {.texture = tex, .sampler = linearFiltering}
                        },
                        .call = IndexedDrawCall(208353, 0, indices),
                        .instanceCount = NUM_BUNNIES_COLUMNS * NUM_BUNNIES_ROWS,
                        .firstInstance = 0
                });
            });

            context.withDefaultRenderTarget([&](auto guard) {
                guard.clear(ClearCommand(ColorRGBA(0.0, 0.1f, 0.0, 1.0), 1.0f));

                guard.draw(pipelines::fullscreen::DrawCmd{
                        .pipeline = quadPipeline,
                        .vertexBindings =  {
                                .perVertex = {.buffer = fullscreenQuad, .offset = 0}
                        },
                        .resourceBindings = {
                                .tex = {.texture = static_cast<const Texture2d &>(dynamic_cast<const OwnedDepthTextureAttachment *>(framebuffer.getDepthAttachment().value())->getTexture()), .sampler = nearestFiltering},
                                .colorTex = {.texture = static_cast<const Texture2d &>(dynamic_cast<const OwnedColorTextureAttachment *>(framebuffer.getColorAttachment(
                                        0).value())->getTexture()), .sampler = nearestFiltering}
                        },
                        .call = NonIndexedDrawCall(6),
                        .instanceCount = 1,
                        .firstInstance = 0
                });

                guard.blit(framebuffer, framebuffer.getRect(), Rect2d::fromOrigin(600, 400), GL_COLOR_BUFFER_BIT,
                        SamplerFilter::NEAREST);
            });

            window.swapBuffers();
            window.pollEvents();

            frame++;

            if (seconds >= 1) {
                cerr << "FPS: " << (double) frame / seconds << endl;
                frame = 0;
                seconds = 0;
            }
        }
    }

    Window::terminate();

    return 0;
}
