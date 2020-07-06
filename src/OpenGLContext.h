#ifndef GAME_ENGINE_OPENGLCONTEXT_H
#define GAME_ENGINE_OPENGLCONTEXT_H

#include "commands.h"
#include "pipeline.h"
#include "Buffer.h"
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
    RasterizationState rasterizer;
    bool depthMask = true;
    optional<GraphicsPipeline> currentPipeline = nullopt;

    DefaultRenderTarget defaultRenderTarget;

    shared_ptr<Program> getProgram(ShaderStages stages);

    void switchProgram(const Program& program);
    void bindBuffer(Buffer &buffer, GLenum target);
    void bindTexture(Texture &texture);
    void bindUniformBuffer(const Buffer &buffer, GLuint index, GLintptr offset, GLsizeiptr size);
    void bindVertexArray(VertexArray& vertexArray);
    void bindFramebuffer(GLuint framebufferId);
    void bindReadFramebuffer(GLuint framebufferId);
    void bindRenderbuffer(Renderbuffer& renderbuffer);
    void switchPipeline(GraphicsPipeline& pipeline);

    template<typename T>
    void switchRenderTarget(T& renderTarget) {
        bindFramebuffer(renderTarget.getId());

        Dimensions2d size = renderTarget.getSize();
        glViewport(0, 0, size.width, size.height);
    }

    // these are called internally by the `RenderTargetGuard`.
    // they implicitly draw & clear whichever surface is currently bound
    void submit(ClearCommand command);
    void submit(DrawCommand command);

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

    Texture buildTexture2D(DataFormat format, Dimensions2d size, bool hasMipMaps);
    void uploadBaseImage2D(Texture& texture, TransferFormat transferFormat, const void *data);

    void setSwapInterval(int nframes);

    Buffer buildBuffer(BufferUsage usage, GLsizeiptr size, const void *data, GLbitfield flags);
    Buffer buildBuffer(BufferUsage usage, GLsizeiptr size, GLbitfield flags);
    GraphicsPipeline buildPipeline(GraphicsPipelineCreateInfo info);
    Renderbuffer buildRenderbuffer(Dimensions2d size, RenderbufferInternalFormat format);
    Framebuffer buildFramebuffer(ColorAttachments colorAttachments, MaybeDepthAttachment depthAttachment, MaybeStencilAttachment stencilAttachment);

    template<typename F>
    void withMappedBuffer(Buffer &buffer, GLintptr offset, GLintptr length, GLbitfield accessFlags, F callback) {
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
    void draw(DrawCommand command);
    template<typename T>
    void blit(T& from, Rect2d source, Rect2d dest, GLuint bits, SamplerFilter filter) {
        context->blit(from, source, dest, bits, filter);
    }
};



#endif //GAME_ENGINE_OPENGLCONTEXT_H
