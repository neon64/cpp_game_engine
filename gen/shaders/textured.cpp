#include "textured.h"
#include <memory>
namespace pipelines { namespace textured {
const char* VERTEX_SHADER = R""(
#version 420
#extension GL_ARB_shading_language_include : enable

layout(location = 0)in vec3 vertexPosition;
layout(location = 1)in vec3 vertexNormal;
layout(location = 2)in vec2 vertexTexCoord;
layout(location = 3)in mat4 modelMatrix;

struct Light {
    float x;
    float y;
    float z;
};

layout(std140, binding = 0)uniform MatrixBlock {
    mat4 viewProjectionMatrix;

};

out vec2 texCoord;
out vec3 normal;

void main(void){

    gl_Position = viewProjectionMatrix * modelMatrix * vec4(vertexPosition, 1.0);
    texCoord = vertexTexCoord;
    normal = vertexNormal;
}
)"";
string VertexShader::getKey() const { return key; }
shared_ptr<Shader> VertexShader::build(OpenGLContext& context) {
    return make_shared<Shader>(std::move(context.buildShader(ShaderType::VERTEX, key, VERTEX_SHADER)));
}
const char* FRAGMENT_SHADER = R""(
#version 420
#extension GL_ARB_shading_language_include : enable

in vec3 normal;
in vec2 texCoord;
layout(location = 0)out vec4 color;

layout(binding = 0)uniform sampler2D diffuseTexture;

void main(void){
    color = vec4(texture(diffuseTexture, texCoord). rgb, 1.0f);
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
    guard.bindVertexBuffer(1, bindings.perInstance.buffer, bindings.perInstance.byteOffset, sizeof(InstanceInput));
}
VertexBindingPipelineState VertexBindingCreateInfo::init(VertexArray& array, OpenGLContext& context) {
    context.withBoundVertexArray(array, [](auto guard) {
        guard.enableAttribute(0);
        guard.enableAttribute(2);
        guard.enableAttribute(1);
        guard.enableAttribute(3);
        guard.enableAttribute(4);
        guard.enableAttribute(5);
        guard.enableAttribute(6);
        guard.setAttributeFormat(0, DataFormat::R32G32B32_SFLOAT, offsetof(VertexInput, position));
        guard.setAttributeFormat(2, DataFormat::R32G32_SFLOAT, offsetof(VertexInput, texCoord));
        guard.setAttributeFormat(1, DataFormat::R32G32B32_SFLOAT, offsetof(VertexInput, normal));
        guard.setAttributeFormat(3, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column0));
        guard.setAttributeFormat(4, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column1));
        guard.setAttributeFormat(5, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column2));
        guard.setAttributeFormat(6, DataFormat::R32G32B32A32_SFLOAT, offsetof(InstanceInput, modelMatrix.column3));
        guard.setAttributeBinding(0, 0);
        guard.setAttributeBinding(2, 0);
        guard.setAttributeBinding(1, 0);
        guard.setAttributeBinding(3, 1);
        guard.setAttributeBinding(4, 1);
        guard.setAttributeBinding(5, 1);
        guard.setAttributeBinding(6, 1);
        guard.setBindingDivisor(1, 1);
    });
    return VertexBindingPipelineState {};
}
void ResourceBindingPipelineState::bindAll(const ResourceBindings& bindings, OpenGLContext& context) {
    context.bindUniformBuffer(bindings.matrixBlock.buffer, 0, bindings.matrixBlock.byteOffset, sizeof(MatrixBlock));
    context.bindTextureAndSampler(0, bindings.diffuseTexture);
}
ResourceBindingPipelineState ResourceBindingCreateInfo::init() {
    return ResourceBindingPipelineState {};
}
}}