#ifndef GAME_ENGINE_OPENGLCONTEXT_H
#define GAME_ENGINE_OPENGLCONTEXT_H

#include "commands.h"
#include "pipeline.h"
#include "Buffer.h"

#include "Window.h"
#include "VertexArray.h"
#include "texturing.h"

#include <glad/glad.h>

#include <unordered_map>
#include <memory>

using namespace std;

class OpenGLContext;

class DefaultRenderTarget : public RenderTarget {
public:
    void activate(OpenGLContext* context);
};

class RenderTargetGuard {
    OpenGLContext* context;
public:
    RenderTargetGuard(OpenGLContext* context);

    void clear(ClearCommand command);
    void draw(DrawCommand command);
};

class OpenGLContext {
private:
    friend class RenderTargetGuard;
    friend class DefaultRenderTarget;

    unordered_map<ShaderStages, shared_ptr<Program>> programCache;
    unordered_map<GLenum, GLuint> boundBuffers;
    unordered_map<GLuint, GLuint> boundUniformBuffers;
    int currentVertexArray = 0;
    int currentProgram = 0;

    bool primitiveRestartEnabled = false;
    RasterizationState rasterizer;
    bool depthMask = true;
    optional<shared_ptr<GraphicsPipeline>> currentPipeline = nullopt;

    DefaultRenderTarget defaultRenderTarget;

    shared_ptr<Program> getProgram(ShaderStages stages);

    void switchProgram(const Program& program);
    void bindBuffer(Buffer &buffer, GLenum target);
    void bindTexture(Texture &texture);
    void bindUniformBuffer(Buffer &buffer, GLuint index, GLintptr offset, GLsizeiptr size);
    void bindVertexArray(VertexArray& vertexArray);
    void bindFramebuffer(GLuint framebufferId);
    void switchPipeline(const shared_ptr<GraphicsPipeline>& pipeline);

    // these are called internally by the `RenderTargetGuard`.
    // they implicitly draw & clear whichever surface is currently bound
    void submit(ClearCommand command);
    void submit(DrawCommand command);

public:
    OpenGLContext(Window &window);

    // can only move, not copyable
    OpenGLContext(const OpenGLContext&) = delete;
    OpenGLContext& operator=(const OpenGLContext&) = delete;
    OpenGLContext(OpenGLContext&& other);

    shared_ptr<Texture> buildTexture2D(DataFormat format, Dimensions2d size, bool hasMipMaps);
    void uploadBaseImage2D(Texture& texture, TransferFormat transferFormat, const void *data);

    void setSwapInterval(int nframes);

    shared_ptr<Buffer> buildBuffer(BufferUsage usage, GLsizeiptr size, const void *data, GLbitfield flags);
    shared_ptr<Buffer> buildBuffer(BufferUsage usage, GLsizeiptr size, GLbitfield flags);

    shared_ptr<GraphicsPipeline> buildPipeline(GraphicsPipelineCreateInfo info);

    template<typename F>
    void withMappedBuffer(Buffer &buffer, GLintptr offset, GLintptr length, GLbitfield accessFlags, F callback) {
        bindBuffer(buffer, GL_ARRAY_BUFFER);

        void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, offset, length, accessFlags);

        callback(ptr);

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    DefaultRenderTarget getDefaultRenderTarget();

    template<typename T, typename F>
    void withRenderTarget(T target, F callback) {
        target.activate(this);
        RenderTargetGuard guard(this);
        callback(guard);
    }

};



#endif //GAME_ENGINE_OPENGLCONTEXT_H
