#ifndef GAME_ENGINE_COMMANDS_H
#define GAME_ENGINE_COMMANDS_H

#include <optional>
#include <glad/glad.h>
#include <memory>
#include <unordered_map>
#include <variant>

#include "buffer.h"
#include "ColorRGBA.h"
#include "texturing.h"
#include "pipeline.h"

using namespace std;

struct ClearCommand {
    const std::optional<ColorRGBA> color;
    const std::optional<GLdouble> depth;
    const std::optional<GLint> stencil;

    ClearCommand(ColorRGBA color);
    ClearCommand(ColorRGBA color, double depth);
};

struct NonIndexedDrawCall {
    const GLuint vertexCount;
    const GLuint firstVertex = 0;

    NonIndexedDrawCall(GLuint vertexCount) : vertexCount(vertexCount) {}
};

struct IndexedDrawCall {
    const GLuint indexCount;
    const GLintptr firstIndex = 0;
    const GLuint firstVertex = 0;
    const IndexBufferRef indexBuffer;

    IndexedDrawCall(GLuint indexCount, GLuint firstIndex, IndexBufferRef indexBuffer)
        : indexCount(indexCount), firstIndex(firstIndex), indexBuffer(indexBuffer) {}
};

template<typename V, typename R>
struct DrawCommand {
    GraphicsPipeline<V, R>& pipeline;

    const V vertexBindings;
    const R resourceBindings;

    const variant<NonIndexedDrawCall, IndexedDrawCall> call;

    const GLuint instanceCount = 1;
    const GLuint firstInstance = 0;
};

using UntypedDrawCommand = DrawCommand<UntypedVertexBindings, UntypedResourceBindings>;

#endif //GAME_ENGINE_COMMANDS_H
