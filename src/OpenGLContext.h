#ifndef GAME_ENGINE_OPENGLCONTEXT_H
#define GAME_ENGINE_OPENGLCONTEXT_H

#include "commands.h"
#include "pipeline.h"
#include "Buffer.h"

#include "Window.h"
#include "VertexArray.h"

#include <glad/glad.h>

#include <unordered_map>
#include <memory>

using namespace std;

class OpenGLContext {
private:
    unordered_map<ShaderStages, shared_ptr<Program>> programCache;
    unordered_map<GLenum, GLuint> boundBuffers;
    unordered_map<GLuint, GLuint> boundUniformBuffers;
    int currentVertexArray = 0;
    int currentProgram = 0;

    bool primitiveRestartEnabled = false;
    RasterizationState rasterizer;
    bool depthMask = true;
    optional<shared_ptr<GraphicsPipeline>> currentPipeline = nullopt;

    shared_ptr<Program> getProgram(ShaderStages stages);

    void switchProgram(const Program& program);
    void bindBuffer(Buffer &buffer, GLenum target);
    void bindUniformBuffer(Buffer &buffer, GLuint index, GLintptr offset, GLsizeiptr size);
    void bindVertexArray(VertexArray& vertexArray);
    void switchPipeline(const shared_ptr<GraphicsPipeline>& pipeline);

public:
    OpenGLContext(Window &window);

    // can only move, not copyable
    OpenGLContext(const OpenGLContext&) = delete;
    OpenGLContext& operator=(const OpenGLContext&) = delete;
    OpenGLContext(OpenGLContext&& other);

    void setSwapInterval(int nframes);

    void setBufferStorage(Buffer &buffer, GLsizeiptr size, const void *data, GLbitfield flags);

    template<typename F>
    void withMappedBuffer(Buffer &buffer, GLintptr offset, GLintptr length, GLbitfield accessFlags, F callback) {
        bindBuffer(buffer, GL_ARRAY_BUFFER);

        void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, offset, length, accessFlags);

        callback(ptr);

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    void submit(ClearCommand command);

    void submit(DrawCommand command);

    shared_ptr<GraphicsPipeline> buildPipeline(GraphicsPipelineCreateInfo info);
};



#endif //GAME_ENGINE_OPENGLCONTEXT_H
