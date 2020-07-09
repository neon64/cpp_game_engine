#ifndef GAME_ENGINE_OPENGLCONTEXT_H
#define GAME_ENGINE_OPENGLCONTEXT_H

#include "commands.h"
#include "pipeline.h"
#include "buffer.h"
#include "Window.h"
#include "VertexArray.h"
#include "texturing.h"
#include "RenderTarget.h"

#include <glad/glad.h>
#include <unordered_map>
#include <memory>

using namespace std;

class RenderTargetGuard;

class OpenGLContext {
private:
    friend class RenderTargetGuard;
    friend class DefaultRenderTarget;

    Window& window;

    unordered_map<ShaderStages, shared_ptr<Program>> programCache;
    unordered_map<GLenum, GLuint> boundBuffers;
    GLuint currentVertexArray = 0;
    GLuint currentProgram = 0;
    GLuint currentFramebuffer = 0;

    bool primitiveRestartEnabled = false;
    RasterizerState rasterizer;
    DepthStencilState depthStencil;
    ColorBlendState colorBlend;

    DefaultRenderTarget defaultRenderTarget;

    shared_ptr<Program> getProgram(ShaderStages stages);

    void switchProgram(const Program& program);
    void bindBuffer(UntypedBuffer &buffer, GLenum target);
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

        bindVertexArray(command.pipeline.vertexArray);

        command.pipeline.vertexPipelineState.bindAll(command.vertexBindings, command.pipeline.vertexArray, *this);
        command.pipeline.resourcesPipelineState.bindAll(command.resourceBindings, *this);

        PrimitiveTopology topology = command.pipeline.inputAssembler.topology;
        performDrawCall(topology, command.call, command.instanceCount, command.firstInstance);
    }

    void performDrawCall(PrimitiveTopology topology, variant<NonIndexedDrawCall, IndexedDrawCall> call, GLuint instanceCount, GLuint firstInstance);

    template<typename T>
    void blit(T& from, Rect2d source, Rect2d dest, GLuint bits, SamplerFilter filter) {
        bindReadFramebuffer(from.getId());
        glBlitFramebuffer(source.origin.x, source.origin.y, source.size.width, source.size.height, dest.origin.x,
                dest.origin.y, dest.size.width, dest.size.height,
                bits, filter == SamplerFilter::NEAREST ? GL_NEAREST : GL_LINEAR);
    }

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

    template<typename V, typename R>
    GraphicsPipeline<V, R> buildPipeline(GraphicsPipelineCreateInfo<V, R> info) {
        shared_ptr<Program> program = getProgram(info.shaders);
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

    void bindUniformBuffer(const UntypedBuffer &buffer, GLuint index, GLintptr offset, GLsizeiptr size);
    void bindTextureAndSampler(uint32_t unit, const Texture& texture, const Sampler& sampler);

    void enableVertexArrayAttribute(VertexArray& array, GLuint location);
    void setVertexArrayAttributeFormat(VertexArray& array, GLuint location, DataFormat format, GLuint offset);
    void setVertexArrayAttributeBinding(VertexArray& array, GLuint location, GLuint binding);
    void setVertexArrayBindingDivisor(VertexArray& array, GLuint binding, GLuint divisor);
    void bindVertexBuffer(VertexArray& array, GLuint index, const UntypedBuffer& buffer, uint32_t offset, uint32_t stride);

    Renderbuffer buildRenderbuffer(Dimensions2d size, RenderbufferInternalFormat format);
    Framebuffer buildFramebuffer(ColorAttachments colorAttachments, MaybeDepthAttachment depthAttachment, MaybeStencilAttachment stencilAttachment);

    template<typename F>
    void withMappedBuffer(UntypedBuffer &buffer, GLintptr offset, GLintptr length, GLbitfield accessFlags, F callback) {
        bindBuffer(buffer, GL_ARRAY_BUFFER);

        void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, offset, length, accessFlags);

        callback(ptr);

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
