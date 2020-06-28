#ifndef GAME_ENGINE_OPENGLCONTEXT_H
#define GAME_ENGINE_OPENGLCONTEXT_H

#include "commands.h"
#include "pipeline.h"

#include "Window.h"
#include "VertexArray.h"

#include <unordered_map>
#include <memory>

using namespace std;

class OpenGLContext {
private:
    // unordered_map<ShaderStages, shared_ptr<Program>> programCache;
    int currentVertexArray = 0;

public:
    OpenGLContext(Window &window);

    // can only move, not copyable
    OpenGLContext(const OpenGLContext&) = delete;
    OpenGLContext& operator=(const OpenGLContext&) = delete;
    OpenGLContext(OpenGLContext&& other);

    void bindVertexArray(VertexArray& vertexArray);

    void submit(ClearCommand command);

    void submit(DrawCommand command);

    shared_ptr<GraphicsPipeline> buildPipeline(GraphicsPipelineCreateInfo info);
};



#endif //GAME_ENGINE_OPENGLCONTEXT_H
