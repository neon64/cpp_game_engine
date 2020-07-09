
#pragma once

#include "../pipeline.h"
#include "../../gen/shaders/textured.h"
#include <glm/glm.hpp>

namespace pipelines { namespace textured {

pipelines::textured::PipelineCreateInfo pipeline();

} }

//using Vertex = pipelines::textured::VertexInput;
//using InstanceAttrs = pipelines::textured::InstanceInput;
//using Uniforms = pipelines::textured::MatrixBlock;

//struct Vertex {
//    glm::vec3 pos;
//    //float pad;
//    glm::vec3 norm;
//    //float pad2;
//    glm::vec2 texCoord;
//    //float pad3;
//    //float pad4;
//};
//
//struct InstanceAttrs {
//    glm::mat4 mvp;
//};
//
//struct Uniforms {
//    glm::mat4 viewProj;
//};

/*UntypedVertexInputCreateInfo {
        .bindings = {
                {.binding=0, .stride=sizeof(pipelines::textured::VertexInput), .inputRate=InputRate::PER_VERTEX},
                {.binding=1, .stride=sizeof(pipelines::textured::InstanceInput), .inputRate=InputRate::PER_INSTANCE}
        },
        .attributes = {
                {.location=0, .binding=0, .format=DataFormat::R32G32B32_SFLOAT, .offset=offsetof(Vertex,
                                                                                                 position)},
                {.location=1, .binding=0, .format=DataFormat::R32G32B32_SFLOAT, .offset=offsetof(Vertex,
                                                                                                 normal)},
                {.location=2, .binding=0, .format=DataFormat::R32G32_SFLOAT, .offset=offsetof(Vertex,
                                                                                              texCoord)},
                {.location=3, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=offsetof(
                        InstanceAttrs, modelMatrix)},
                {.location=4, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=
                offsetof(InstanceAttrs, modelMatrix) + 4 * sizeof(GLfloat)},
                {.location=5, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=
                offsetof(InstanceAttrs, modelMatrix) + 8 * sizeof(GLfloat)},
                {.location=6, .binding=1, .format=DataFormat::R32G32B32A32_SFLOAT, .offset=
                offsetof(InstanceAttrs, modelMatrix) + 12 * sizeof(GLfloat)}
        }
}*/