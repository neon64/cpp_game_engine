#ifndef GAME_ENGINE_COMMANDS_H
#define GAME_ENGINE_COMMANDS_H

#include <optional>
#include <glad/glad.h>
#include <memory>
#include <unordered_map>
#include <variant>

#include "Buffer.h"
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

struct VertexBufferBinding {
    const shared_ptr<Buffer> buffer;
    const uint32_t offset;
};

struct UniformBufferBinding {
    const shared_ptr<Buffer> buffer;
    const uint32_t offset;
    const uint32_t size;
};

struct TextureBinding {
    const shared_ptr<Texture> texture;
    const shared_ptr<Sampler> sampler;
};

struct NonIndexedDrawCall {
    const GLuint vertexCount;
    const GLuint firstVertex = 0;

    NonIndexedDrawCall(GLuint vertexCount) : vertexCount(vertexCount) {}
};

struct IndexedDrawCall {
    const GLuint indexCount;
    const GLintptr firstIndex = 0;
    const IndexBufferRef indexBuffer;

    IndexedDrawCall(GLuint indexCount, GLuint firstIndex, IndexBufferRef indexBuffer)
        : indexCount(indexCount), firstIndex(firstIndex), indexBuffer(indexBuffer) {}
};

struct DrawCommand {
    const shared_ptr<GraphicsPipeline> pipeline;

    // TODO: optimise this
    const unordered_map<uint32_t, VertexBufferBinding> vertexBuffers;
    const unordered_map<uint32_t, UniformBufferBinding> uniformBuffers;
    const unordered_map<uint32_t, TextureBinding> textures;

    const variant<NonIndexedDrawCall, IndexedDrawCall> call;

    const GLuint instanceCount = 1;
    const GLuint firstInstance = 0;
};


#endif //GAME_ENGINE_COMMANDS_H
