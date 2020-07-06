
#ifndef GAME_ENGINE_PIPELINE_H
#define GAME_ENGINE_PIPELINE_H

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include <tuple>
#include <glad/glad.h>
#include <unordered_map>

#include "Program.h"
#include "VertexArray.h"

using namespace std;

enum PrimitiveTopology {
    POINTS = GL_POINTS,
    LINES = GL_LINES,
    TRIANGLES = GL_TRIANGLES,
    TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
    LINE_STRIP = GL_LINE_STRIP,
    LINE_LOOP = GL_LINE_LOOP
};

enum InputRate {
    PER_VERTEX,
    PER_INSTANCE
};

enum DataFormat {
    R8G8B8_UINT,
    R8G8B8A8_UINT,
    R8G8B8_SRGB,
    R8G8B8A8_SRGB,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    R32G32B32A32_SFLOAT,
    D16_UNORM,
    D24_UNORM,
    D32_SFLOAT
};

struct VertexInputBinding {
    uint32_t binding;
    uint32_t stride;
    InputRate inputRate;
};

struct VertexInputAttribute {
    uint32_t location;
    uint32_t binding;
    DataFormat format;
    uint32_t offset;
};

struct VertexInputState {
    vector<VertexInputBinding> bindings;
    vector<VertexInputAttribute> attributes;
};

struct InputAssemblerState {
    PrimitiveTopology topology;
    bool primitiveRestartEnable = false;
};

enum PolygonMode {
    FILL = GL_FILL,
    LINE = GL_LINE,
    POINT = GL_POINT
};

enum CullMode {
    FRONT = GL_FRONT,
    BACK = GL_BACK,
    FRONT_AND_BACK = GL_FRONT_AND_BACK
};

enum FrontFace {
    CLOCKWISE = GL_CW,
    COUNTER_CLOCKWISE = GL_CCW
};

struct RasterizationState {
    // bool depthClampEnable = false;
    // bool rasterizerDiscardEnable = false;
    PolygonMode polygonMode = PolygonMode::FILL;
    optional<CullMode> culling = nullopt;
    FrontFace frontFace = FrontFace::COUNTER_CLOCKWISE;
    // TODO: support the rest of these
    // bool  depthBiasEnable;
    // float depthBiasConstantFactor;
    // float depthBiasClamp;
    // float depthBiasSlopeFactor;
    // float lineWidth = 1;
};

enum ComparisonFunction {
    NEVER = GL_NEVER,
    LESS_THAN = GL_LESS,
    EQUAL = GL_EQUAL,
    LESS_THAN_OR_EQUAL_TO = GL_LEQUAL,
    GREATER_THAN = GL_GREATER,
    GREATER_THAN_OR_EQUAL_TO = GL_GEQUAL,
    NOT_EQUAL = GL_NOTEQUAL,
    ALWAYS = GL_ALWAYS
};

struct DepthStencilState {
    optional<ComparisonFunction> depthTest = nullopt;
    bool depthMask = true;
    // TODO: support the rest of these
    // bool depthBoundsTestEnable;
    // bool stencilTestEnable;
    // VkStencilOpState front;
    // VkStencilOpState back;
    // float minDepthBounds;
    // float maxDepthBounds;
};

enum BlendingFactor {
    ZERO = GL_ZERO, ONE = GL_ONE,
    SRC_COLOR = GL_SRC_COLOR, ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR,
    DST_COLOR = GL_DST_COLOR, ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR,
    SRC_ALPHA = GL_SRC_ALPHA, ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA
//    DST_ALPHA, ONE_MINUS_DST_ALPHA,
//    CONSTANT_COLOR, ONE_MINUS_CONSTANT_COLOR,
//    CONSTANT_ALPHA, ONE_MINUS_CONSTANT_ALPHA,
//    SRC_ALPHA_SATURATE,
//    SRC1_COLOR, ONE_MINUS_SRC1_COLOR,
//    SRC1_ALPHA, ONE_MINUS_SRC1_ALPHA;
};

enum BlendingEquation {
    ADD = GL_FUNC_ADD,
    SUBTRACT = GL_FUNC_SUBTRACT,
    REVERSE_SUBTRACT = GL_FUNC_REVERSE_SUBTRACT,
    MIN = GL_MIN,
    MAX = GL_MAX
};

struct Blending {
    BlendingEquation equation;
    BlendingFactor srcFactor;
    BlendingFactor dstFactor;
};

struct ColorBlendPerAttachment {
    optional<Blending> blending = nullopt;
    tuple<bool, bool, bool, bool> mask = tuple(true, true, true, true);
};

struct ColorBlendState {
    unordered_map<int, ColorBlendPerAttachment> attachments;
};

struct ShaderStages {
    shared_ptr<Shader> vertex;
    shared_ptr<Shader> fragment;
    // TODO: optional other stages

    bool operator==(const ShaderStages &other) const {
        return (vertex == other.vertex
              && fragment == other.fragment);
    }
};

// from https://en.cppreference.com/w/cpp/utility/hash
namespace std {

    template <>
    struct hash<ShaderStages> {
        std::size_t operator()(const ShaderStages& k) const {
            using std::size_t;
            using std::hash;
            using std::string;

            // should maybe use boost::hash_combine instead
            return ((hash<shared_ptr<Shader>>()(k.vertex)
                     ^ (hash<shared_ptr<Shader>>()(k.fragment) << 1)) >> 1);
        }
    };

}

struct GraphicsPipelineCreateInfo {
    ShaderStages shaders;
    VertexInputState vertexInput;
    InputAssemblerState inputAssembler;
    RasterizationState rasterizer;
    DepthStencilState depthStencil;
    ColorBlendState colorBlend;
};

class GraphicsPipeline {
public:
    shared_ptr<Program> program;
    VertexArray vertexArray;
    vector<VertexInputBinding> vertexInputBindings;
    InputAssemblerState inputAssembler;
    // TesselationState;
    // ViewportState; - will use the default viewport
    RasterizationState rasterizer;
    // MultisampleState;
    DepthStencilState depthStencil;
    ColorBlendState colorBlend;
    // DynamicState;

public:
    GraphicsPipeline(shared_ptr<Program> program, VertexArray&& vertexArray, vector<VertexInputBinding> vertexInputBindings, InputAssemblerState inputAssembler, RasterizationState rasterizer, DepthStencilState depthStencil, ColorBlendState colorBlend);
};

#endif //GAME_ENGINE_PIPELINE_H
