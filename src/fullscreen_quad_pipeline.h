
#ifndef GAME_ENGINE_FULLSCREEN_QUAD_PIPELINE_H
#define GAME_ENGINE_FULLSCREEN_QUAD_PIPELINE_H

namespace fullscreen_quad {

const char *VERTEX_SHADER =

#include "../gen/shaders/fullscreen.vert"
;

const char *FRAGMENT_SHADER =

#include "../gen/shaders/fullscreen.frag"
;

GraphicsPipelineCreateInfo pipeline() {
    auto vertexShader = Shader::build(ShaderType::VERTEX, "awesome new shader", VERTEX_SHADER);
    auto fragmentShader = Shader::build(ShaderType::FRAGMENT, "awesome new fragment shader", FRAGMENT_SHADER);

    GraphicsPipelineCreateInfo info = {
            .shaders = {
                    .vertex = vertexShader,
                    .fragment = fragmentShader
            },
            .vertexInput = {
                    .bindings = {
                            {.binding=0, .stride=sizeof(glm::vec3), .inputRate=InputRate::PER_VERTEX}
                    },
                    .attributes = {
                            {.location=0, .binding=0, .format=DataFormat::R32G32B32_SFLOAT, .offset=0}
                    }
            },
            .inputAssembler {
                    .topology = PrimitiveTopology::TRIANGLES,
                    .primitiveRestartEnable = false
            },
            .rasterizer {
                    //.polygonMode = PolygonMode::FILL,
                    //.culling = make_optional(CullMode::BACK)
            },
            .depthStencil {
                    //.depthTest = make_optional(ComparisonFunction::LESS_THAN_OR_EQUAL_TO),
                    .depthMask = true
            },
            .colorBlend {
            },
    };

    return info;
};

}

#endif //GAME_ENGINE_FULLSCREEN_QUAD_PIPELINE_H
