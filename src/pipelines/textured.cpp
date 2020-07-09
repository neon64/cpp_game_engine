
#include "textured.h"

namespace pipelines { namespace textured {

PipelineCreateInfo pipeline() {
    auto vertexShader = Shader::build(ShaderType::VERTEX, "textured.vert", VERTEX_SHADER);
    auto fragmentShader = Shader::build(ShaderType::FRAGMENT, "textured.frag", FRAGMENT_SHADER);

    return {
            .shaders = {
                .vertex = vertexShader,
                .fragment = fragmentShader
            },
            .depthStencil {
                .depthTest = make_optional(ComparisonFunction::LESS_THAN_OR_EQUAL_TO),
            },
            .colorBlend {
                .attachments = { { 0, { .blending = Blending::ALPHA }}}
            },
    };
}

}}