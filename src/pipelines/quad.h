
#pragma once

#include "../pipeline.h"
#include <glm/glm.hpp>
#include "../../gen/shaders/fullscreen.h"

namespace pipelines { namespace fullscreen {

PipelineCreateInfo pipeline() {
    auto vertexShader = Shader::build(ShaderType::VERTEX, "fullscreen.vert", VERTEX_SHADER);
    auto fragmentShader = Shader::build(ShaderType::FRAGMENT, "fullscreen.frag", FRAGMENT_SHADER);

    return {
        .shaders = {
                .vertex = vertexShader,
                .fragment = fragmentShader
        },
        .colorBlend {
            .attachments = { { 0, { .blending = Blending::DISABLED } } }
        },
    };
};

} }