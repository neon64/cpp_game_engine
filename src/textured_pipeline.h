
#ifndef GAME_ENGINE_TEXTURED_PIPELINE_H
#define GAME_ENGINE_TEXTURED_PIPELINE_H

namespace textured_pipeline {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texCoord;
};

struct InstanceAttrs {
    glm::mat4 mvp;
};

struct Uniforms {
    glm::mat4 viewProj;
};

const char *VERTEX_SHADER =

#include "../gen/shaders/basicTest.vert"
;

const char *FRAGMENT_SHADER =

#include "../gen/shaders/basicTest.frag"
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
                            {.binding=0, .stride=sizeof(Vertex), .inputRate=InputRate::PER_VERTEX},
                            {.binding=1, .stride=sizeof(InstanceAttrs), .inputRate=InputRate::PER_INSTANCE}
                    },
                    .attributes = {
                            {.location=0, .binding=0, .format=DataFormat::R32G32B32_SFLOAT, .offset=offsetof(Vertex,
                                                                                                             pos)},
                            {.location=1, .binding=0, .format=DataFormat::R32G32B32_SFLOAT, .offset=offsetof(Vertex,
                                                                                                             norm)},
                            {.location=2, .binding=0, .format=DataFormat::R32G32_SFLOAT, .offset=offsetof(Vertex,
                                                                                                          texCoord)},
                            {.location=3, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=offsetof(
                                    InstanceAttrs, mvp)},
                            {.location=4, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=
                            offsetof(InstanceAttrs, mvp) + 4 * sizeof(GLfloat)},
                            {.location=5, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=
                            offsetof(InstanceAttrs, mvp) + 8 * sizeof(GLfloat)},
                            {.location=6, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=
                            offsetof(InstanceAttrs, mvp) + 12 * sizeof(GLfloat)}
                    }
            },
            .inputAssembler {
                    .topology = PrimitiveTopology::TRIANGLES,
                    .primitiveRestartEnable = false
            },
            .rasterizer {
                    .polygonMode = PolygonMode::FILL,
                    //.culling = make_optional(CullMode::BACK)
            },
            .depthStencil {
                    .depthTest = make_optional(ComparisonFunction::LESS_THAN_OR_EQUAL_TO),
                    .depthMask = true
            },
            .colorBlend {
            },
    };

    return info;
};

}

#endif //GAME_ENGINE_TEXTURED_PIPELINE_H
