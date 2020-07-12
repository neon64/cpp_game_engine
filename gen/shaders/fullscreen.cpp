#include "fullscreen.h"
#include <memory>
namespace pipelines { namespace fullscreen {
const char* VERTEX_SHADER = R""(
#version 420
#extension GL_ARB_shading_language_include : enable

layout(location = 0)in vec3 vertexPosition;

out vec2 texCoord;

void main(){
    texCoord =(vertexPosition . xy + 1.0)* 0.5;
    gl_Position = vec4(vertexPosition, 1.0);
}
)"";
string VertexShader::getKey() const { return key; }
shared_ptr<Shader> VertexShader::build(OpenGLContext& context) {
    return make_shared<Shader>(std::move(context.buildShader(ShaderType::VERTEX, key, VERTEX_SHADER)));
}
const char* FRAGMENT_SHADER = R""(
#version 420
#extension GL_ARB_shading_language_include : enable

layout(binding = 0)uniform sampler2D tex;
layout(binding = 1)uniform sampler2D colorTex;

const float zNear = 0.01;
const float zFar = 10.0;

in vec2 texCoord;
out vec4 color;

void main(){
    float z_b = texture(tex, texCoord). r;
    vec3 c = texture(colorTex, texCoord). rgb;

    float z_n = 2.0 * z_b - 1.0;
    float z_e = 2.0 * zNear * zFar /(zFar + zNear - z_n *(zFar - zNear));

    color = vec4(vec3(z_e), 1.0);
}
)"";
string FragmentShader::getKey() const { return key; }
shared_ptr<Shader> FragmentShader::build(OpenGLContext& context) {
    return make_shared<Shader>(std::move(context.buildShader(ShaderType::FRAGMENT, key, FRAGMENT_SHADER)));
}
Shaders::Shaders(ShaderCache* cache) : cache(*cache) {}
Shaders::Shaders(ShaderCache& cache) : cache(cache) {}
ShaderStages Shaders::getStages() const {
    return {
    .vertex = cache.get(VertexShader {}),
    .fragment = cache.get(FragmentShader {}),
    };
};
void VertexBindingPipelineState::bindAll(const VertexBindings& bindings, BoundVertexArrayGuard& guard, OpenGLContext& context) {
    guard.bindVertexBuffer(0, bindings.perVertex.buffer, bindings.perVertex.byteOffset, sizeof(VertexInput));
}
VertexBindingPipelineState VertexBindingCreateInfo::init(VertexArray& array, OpenGLContext& context) {
    context.withBoundVertexArray(array, [](auto guard) {
        guard.enableAttribute(0);
        guard.setAttributeFormat(0, DataFormat::R32G32B32_SFLOAT, offsetof(VertexInput, position));
        guard.setAttributeBinding(0, 0);
    });
    return VertexBindingPipelineState {};
}
void ResourceBindingPipelineState::bindAll(const ResourceBindings& bindings, OpenGLContext& context) {
    context.bindTextureAndSampler(0, bindings.tex);
    context.bindTextureAndSampler(1, bindings.colorTex);
}
ResourceBindingPipelineState ResourceBindingCreateInfo::init() {
    return ResourceBindingPipelineState {};
}
}}