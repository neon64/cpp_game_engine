#ifndef GAME_ENGINE_OPENGLCONTEXT_H
#define GAME_ENGINE_OPENGLCONTEXT_H

#include "commands.h"
#include "pipeline.h"
#include "buffer.h"
#include "../Window.h"
#include "VertexArray.h"
#include "texturing.h"
#include "RenderTarget.h"

#include <span>
#include <glad/glad.h>
#include <unordered_map>
#include <memory>

using namespace std;

class RenderTargetGuard;

// represents OpenGL state in which a given Vertex Array Object is bound.
// allows making calls to modify the VAO state, without the EXT_direct_state_access extension.
class BoundVertexArrayGuard {
    VertexArray& array;

public:
    BoundVertexArrayGuard(VertexArray& array);

    void enableAttribute(GLuint location);
    void setAttributeFormat(GLuint location, DataFormat format, GLuint offset);
    void setAttributeBinding(GLuint location, GLuint binding);
    void setBindingDivisor(GLuint binding, GLuint divisor);
    void bindIndexBuffer(const UntypedBuffer& buffer);
    void bindVertexBuffer(GLuint binding, const UntypedBuffer& buffer, uint32_t offset, uint32_t stride);
};

struct CurrentUniformBufferBinding {
    GLuint bufferId;
    uint32_t byteOffset;
    uint32_t size;

    bool operator==(const CurrentUniformBufferBinding& other) {
        return bufferId == other.bufferId && byteOffset == other.byteOffset && size == other.size;
    }
};

class OpenGLContext {
private:
    friend class RenderTargetGuard;
    friend class DefaultRenderTarget;

    Window& window;

    unordered_map<ShaderStages, shared_ptr<Program>> programCache;
    GLuint boundArrayBuffer;
    GLuint currentVertexArray = 0;
    GLuint currentProgram = 0;
    unordered_map<GLuint, CurrentUniformBufferBinding> boundUniformBuffers;
    GLuint currentFramebuffer = 0;

    bool primitiveRestartEnabled = false;
    RasterizerState rasterizer;
    DepthStencilState depthStencil;
    ColorBlendState colorBlend;

    DefaultRenderTarget defaultRenderTarget;

    shared_ptr<Program> getProgram(ShaderStages stages);

    void switchProgram(const Program& program);
    void bindArrayBuffer(const UntypedBuffer &buffer);
    void bindTexture(Texture &texture);
    void bindVertexArray(VertexArray& vertexArray);
    void bindFramebuffer(GLuint framebufferId);
    void bindReadFramebuffer(GLuint framebufferId);
    void bindRenderbuffer(Renderbuffer& renderbuffer);

    void switchInputAssemblerState(InputAssemblerState& newInputAssembler);
    void switchRasterizerState(RasterizerState& newRasterizer);
    void switchDepthStencilState(DepthStencilState& newDepthStencil);
    void switchColorBlendState(ColorBlendState& newColorBlend);

    template<typename T>
    void switchRenderTarget(T& renderTarget) {
        bindFramebuffer(renderTarget.getId());

        Dimensions2d size = renderTarget.getSize();
        glViewport(0, 0, size.width, size.height);
    }

    // these are called internally by the `RenderTargetGuard`.
    // they implicitly draw & clear whichever surface is currently bound
    void submit(ClearCommand command);

    template<typename V, typename R>
    void submit(DrawCommand<V, R> command) {
        switchProgram(*command.pipeline.program);

        switchInputAssemblerState(command.pipeline.inputAssembler);
        switchRasterizerState(command.pipeline.rasterizer);
        switchDepthStencilState(command.pipeline.depthStencil);
        switchColorBlendState(command.pipeline.colorBlend);

        withBoundVertexArray(command.pipeline.vertexArray, [command, this](auto guard) {
            command.pipeline.vertexPipelineState.bindAll(command.vertexBindings, guard, *this);
            command.pipeline.resourcesPipelineState.bindAll(command.resourceBindings, *this);

            PrimitiveTopology topology = command.pipeline.inputAssembler.topology;
            performDrawCall(topology, command.call, command.instanceCount, command.firstInstance, guard);
        });
    }

    void performDrawCall(PrimitiveTopology topology, variant<NonIndexedDrawCall, IndexedDrawCall> call, GLuint instanceCount, GLuint firstInstance, BoundVertexArrayGuard guard);

    template<typename T>
    void blit(T& from, Rect2d source, Rect2d dest, GLuint bits, SamplerFilter filter) {
        bindReadFramebuffer(from.getId());
        glBlitFramebuffer(source.origin.x, source.origin.y, source.size.width, source.size.height, dest.origin.x,
                dest.origin.y, dest.size.width, dest.size.height,
                bits, filter == SamplerFilter::NEAREST ? GL_NEAREST : GL_LINEAR);
    }

    void bindTextureAndSampler(uint32_t unit, const Texture& texture, const Sampler& sampler);

public:
    OpenGLContext(Window &window);

    // can only move, not copyable
    OpenGLContext(const OpenGLContext&) = delete;
    OpenGLContext& operator=(const OpenGLContext&) = delete;
    OpenGLContext(OpenGLContext&& other);

    Texture2d buildTexture2D(DataFormat format, Dimensions2d size, bool hasMipMaps);
    void uploadBaseImage2D(Texture2d& texture, TransferFormat transferFormat, const void *data);

    void setSwapInterval(int nframes);

    UntypedBuffer buildBuffer(BufferUsage usage, GLsizeiptr size, const void *data, GLbitfield flags);
    UntypedBuffer buildBuffer(BufferUsage usage, GLsizeiptr size, GLbitfield flags);

    template<typename T>
    Buffer<T> buildWritableBuffer(BufferUsage usage) {
        auto buffer = buildBuffer(usage, sizeof(T), GL_MAP_WRITE_BIT);
        return Buffer<T>(std::move(buffer));
    }

    template<typename T,  std::size_t Extent = std::dynamic_extent>
    ArrayBuffer<T> buildStaticArrayBuffer(BufferUsage usage, span<T, Extent> data) {
        auto buffer = buildBuffer(usage, data.size_bytes(), data.data(), 0);
        return ArrayBuffer<T>(std::move(buffer));
    }

    template<typename T>
    ArrayBuffer<T> buildWritableArrayBuffer(BufferUsage usage, size_t numElements) {
        auto buffer = buildBuffer(usage, sizeof(T) * numElements, GL_MAP_WRITE_BIT);
        return ArrayBuffer<T>(std::move(buffer));
    }

    Shader buildShader(ShaderType type, std::string description, const std::string_view &source);

    template<typename V, typename R, typename S>
    GraphicsPipeline<V, R> buildPipeline(GraphicsPipelineCreateInfo<V, R, S> info) {
        shared_ptr<Program> program = getProgram(info.shaders.getStages());
        VertexArray vertexArray = VertexArray::build();

        bindVertexArray(vertexArray);

        typename V::PipelineState vertexPipelineState = info.vertexInput.init(vertexArray, *this);
        typename R::PipelineState resourcePipelineState = info.resourceBindings.init();

        return GraphicsPipeline<V, R>(
            program,
            std::move(vertexArray),
            vertexPipelineState,
            resourcePipelineState,
            info.inputAssembler,
            info.rasterizer,
            info.depthStencil,
            info.colorBlend
        );
    }

    void bindUniformBuffer(const UntypedBuffer &buffer, GLuint index, GLintptr byteOffset, GLsizeiptr size);

    template<typename T>
    void bindTextureAndSampler(uint32_t unit, const TextureBinding<T>& binding) {
        bindTextureAndSampler(unit, binding.texture, binding.sampler);
    }

    template<typename F>
    void withBoundVertexArray(VertexArray& array, F callback) {
        bindVertexArray(array);
        callback(BoundVertexArrayGuard(array));
    }

    Renderbuffer buildRenderbuffer(Dimensions2d size, RenderbufferInternalFormat format);
    Framebuffer buildFramebuffer(ColorAttachments colorAttachments, MaybeDepthAttachment depthAttachment, MaybeStencilAttachment stencilAttachment);

    template<typename B, typename F>
    void withMappedBuffer(B binding, GLbitfield accessFlags, F callback) {
        bindArrayBuffer(binding.buffer);

        void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, binding.byteOffset, binding.getSize(), accessFlags);

        callback(binding.convert(ptr));

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    DefaultRenderTarget &getDefaultRenderTarget();

    template<typename T, typename F>
    void withRenderTarget(T& target, F callback) {
        switchRenderTarget(target);
        RenderTargetGuard guard(this);
        callback(guard);
    }

    template<typename F>
    void withDefaultRenderTarget(F callback) {
        withRenderTarget(getDefaultRenderTarget(), callback);
    }

};



class RenderTargetGuard {
    OpenGLContext* context;
public:
    RenderTargetGuard(OpenGLContext* context);

    void clear(ClearCommand command);
    template<typename V, typename R>
    void draw(DrawCommand<V, R> command) {
        context->submit(command);
    }
    template<typename T>
    void blit(T& from, Rect2d source, Rect2d dest, GLuint bits, SamplerFilter filter) {
        context->blit(from, source, dest, bits, filter);
    }
};



#endif //GAME_ENGINE_OPENGLCONTEXT_H
