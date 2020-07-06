
#include "pipeline.h"


GraphicsPipeline::GraphicsPipeline(shared_ptr<Program> program, VertexArray&& vertexArray, vector<VertexInputBinding> vertexInputBindings,
        InputAssemblerState inputAssembler, RasterizationState rasterizer,
        DepthStencilState depthStencil, ColorBlendState colorBlend) : program(program), vertexArray(std::move(vertexArray)), vertexInputBindings(vertexInputBindings), inputAssembler(inputAssembler), rasterizer(rasterizer), depthStencil(depthStencil), colorBlend(colorBlend) {

}
