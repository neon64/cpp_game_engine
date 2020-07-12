
#ifndef GAME_ENGINE_PIPELINE_H
#define GAME_ENGINE_PIPELINE_H

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include <tuple>
#include <glad/glad.h>
#include <unordered_map>

#include "ColorRGBA.h"
#include "texturing.h"
#include "buffer.h"
#include "Program.h"
#include "VertexArray.h"

using namespace std;

class OpenGLContext;
class BoundVertexArrayGuard;

enum class PrimitiveTopology {
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

struct InputAssemblerState {
    PrimitiveTopology topology = PrimitiveTopology::TRIANGLES;
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

struct RasterizerState {
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
    bool depthWrite = true;
    // TODO: support the rest of these
    // bool depthBoundsTestEnable;
    // bool stencilTestEnable;
    // VkStencilOpState front;
    // VkStencilOpState back;
    // float minDepthBounds;
    // float maxDepthBounds;

    static DepthStencilState LESS_THAN_OR_EQUAL_TO;
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

struct BlendingFunction {
    BlendingEquation equation;
    BlendingFactor srcFactor;
    BlendingFactor dstFactor;

    static BlendingFunction ALPHA;
    static BlendingFunction ADDITIVE;

    bool operator==(const BlendingFunction& other) const;

};

struct Blending {
    BlendingFunction color;
    BlendingFunction alpha;

    static optional<Blending> DISABLED;
    static optional<Blending> ALPHA;
    static optional<Blending> ADDITIVE;

    bool operator==(const Blending& other) const;
};

struct ColorBlendPerAttachment {
    optional<Blending> blending = nullopt;
    tuple<bool, bool, bool, bool> colorMask = tuple(true, true, true, true);

    bool operator==(const ColorBlendPerAttachment& other) const;
};

struct ColorBlendState {
    unordered_map<int, ColorBlendPerAttachment> attachments;
    ColorRGBA constants = ColorRGBA(0, 0, 0, 0);
};

struct ShaderStages {
    shared_ptr<Shader> vertex;
    shared_ptr<Shader> fragment;
    // TODO: optional other stages

    bool operator==(const ShaderStages &other) const {
        return (vertex == other.vertex
                && fragment == other.fragment);
    }

    ShaderStages getStages() const {
        return *this;
    }
};

// from https://en.cppreference.com/w/cpp/utility/hash
namespace std {

template<>
struct hash<ShaderStages> {
    std::size_t operator()(const ShaderStages &k) const {
        using std::size_t;
        using std::hash;
        using std::string;

        // should maybe use boost::hash_combine instead
        return ((hash<shared_ptr<Shader>>()(k.vertex)
                 ^ (hash<shared_ptr<Shader>>()(k.fragment) << 1)) >> 1);
    }
};

}

struct UntypedVertexBindings;
struct UntypedResourceBindings;

struct UntypedVertexBindingPipelineState {
    vector<VertexInputBinding> layout;

    UntypedVertexBindingPipelineState(vector<VertexInputBinding> layout);

    void bindAll(const UntypedVertexBindings& bindings, BoundVertexArrayGuard& guard, OpenGLContext& context);
};

struct UntypedResourceBindingPipelineState {
    void bindAll(const UntypedResourceBindings& bindings, OpenGLContext& context);
};

struct UntypedVertexInputCreateInfo {
    vector<VertexInputBinding> bindings;
    vector<VertexInputAttribute> attributes;

    UntypedVertexBindingPipelineState init(VertexArray& array, OpenGLContext& context);
};

struct UntypedResourceBindingCreateInfo {
    UntypedResourceBindingPipelineState init();
};

struct UntypedVertexBindings {
    const unordered_map<uint32_t, UntypedVertexBufferBinding> bindings;

    UntypedVertexBindings(unordered_map<uint32_t, UntypedVertexBufferBinding> bindings);

    using CreateInfo = UntypedVertexInputCreateInfo;
    using PipelineState = UntypedVertexBindingPipelineState;
};

struct UntypedResourceBindings {
    const unordered_map<uint32_t, UntypedBufferBinding> uniforms;
    const unordered_map<uint32_t, TextureBinding<Texture>> textures;

    using CreateInfo = UntypedResourceBindingCreateInfo;
    using PipelineState = UntypedResourceBindingPipelineState;
};

template<typename V, typename R, typename S>
struct GraphicsPipelineCreateInfo {
    S shaders;
    V::CreateInfo vertexInput {};
    R::CreateInfo resourceBindings {};
    InputAssemblerState inputAssembler;
    RasterizerState rasterizer;
    DepthStencilState depthStencil;
    ColorBlendState colorBlend;
};

using UntypedGraphicsPipelineCreateInfo = GraphicsPipelineCreateInfo<UntypedVertexBindings, UntypedResourceBindings, ShaderStages>;

template<typename V, typename R>
class GraphicsPipeline {
public:
    shared_ptr<Program> program;
    VertexArray vertexArray;
    V::PipelineState vertexPipelineState;
    R::PipelineState resourcesPipelineState;
    InputAssemblerState inputAssembler;

    // TesselationState;
    // ViewportState; - will use the default viewport

    RasterizerState rasterizer;

    // MultisampleState;

    DepthStencilState depthStencil;
    ColorBlendState colorBlend;

    // DynamicState;

public:
    GraphicsPipeline(shared_ptr<Program> program, VertexArray &&vertexArray,
                     V::PipelineState vertexPipelineState, R::PipelineState resourcesPipelineState, InputAssemblerState inputAssembler,
                     RasterizerState rasterizer, DepthStencilState depthStencil, ColorBlendState colorBlend)
            : program(program), vertexArray(std::move(vertexArray)), vertexPipelineState(vertexPipelineState), resourcesPipelineState(resourcesPipelineState),
              inputAssembler(inputAssembler), rasterizer(rasterizer), depthStencil(depthStencil),
              colorBlend(colorBlend) {

    }

    GraphicsPipeline<V, R>* onHeap() {
        return new GraphicsPipeline<V, R>(std::move(*this));
    }
};

using UntypedGraphicsPipeline = GraphicsPipeline<UntypedVertexBindings, UntypedResourceBindings>;

#endif //GAME_ENGINE_PIPELINE_H
