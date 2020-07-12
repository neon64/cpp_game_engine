#pragma once
// autogenerated from GLSL, do not edit
#include <glm/glm.hpp>
#include "../../src/graphics/OpenGLContext.h"
#include "../../src/graphics/commands.h"
#include "../../src/graphics/Shader.h"
#include "../../src/util.h"
#include "../../src/loader/shaders.h"
#include "../../src/graphics/texturing.h"
namespace pipelines { namespace fullscreen {
struct VertexShader {
    string key = "/home/chris/code/game_engine/res/shaders/fullscreen.vert";
    string getKey() const;
    shared_ptr<Shader> build(OpenGLContext& context);
};
struct FragmentShader {
    string key = "/home/chris/code/game_engine/res/shaders/fullscreen.frag";
    string getKey() const;
    shared_ptr<Shader> build(OpenGLContext& context);
};
class Shaders {
    ShaderCache& cache;
public:
    Shaders(ShaderCache& cache);
    Shaders(ShaderCache* cache);
    ShaderStages getStages() const;
};
struct VertexInput {
    glm::vec3 position;
};
struct VertexBindingPipelineState;
struct VertexBindingCreateInfo;
struct VertexBindings {
    const VertexBufferBinding<VertexInput> perVertex;
    using CreateInfo = VertexBindingCreateInfo;
    using PipelineState = VertexBindingPipelineState;
};
struct VertexBindingPipelineState {
    void bindAll(const VertexBindings& bindings, BoundVertexArrayGuard& guard, OpenGLContext& context);
};
struct VertexBindingCreateInfo {
    VertexBindingPipelineState init(VertexArray& array, OpenGLContext& context);
};
struct ResourceBindingPipelineState;
struct ResourceBindingCreateInfo;
struct ResourceBindings {
    const TextureBinding<Texture2d> tex;
    const TextureBinding<Texture2d> colorTex;
    using CreateInfo = ResourceBindingCreateInfo;
    using PipelineState = ResourceBindingPipelineState;
};
struct ResourceBindingPipelineState {
    void bindAll(const ResourceBindings& bindings, OpenGLContext& context);
};
struct ResourceBindingCreateInfo {
    ResourceBindingPipelineState init();
};
using Pipeline = GraphicsPipeline<VertexBindings, ResourceBindings>;
using Create = GraphicsPipelineCreateInfo<VertexBindings, ResourceBindings, Shaders>;
using DrawCmd = DrawCommand<VertexBindings, ResourceBindings>;
}}