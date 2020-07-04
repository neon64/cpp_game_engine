
#include "pipeline.h"


GraphicsPipeline::GraphicsPipeline(shared_ptr<Program> program, shared_ptr<VertexArray> vertexArray, vector<VertexInputBinding> vertexInputBindings,
        InputAssemblerState inputAssembler, RasterizationState rasterizer,
        DepthStencilState depthStencil, ColorBlendState colorBlend) : program(program), vertexArray(vertexArray), vertexInputBindings(vertexInputBindings), inputAssembler(inputAssembler), depthStencil(depthStencil), colorBlend(colorBlend) {

}
