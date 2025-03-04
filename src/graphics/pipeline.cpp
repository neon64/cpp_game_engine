
#include "pipeline.h"

#include "OpenGLContext.h"

UntypedVertexBindings::UntypedVertexBindings(unordered_map<uint32_t, UntypedVertexBufferBinding> bindings) : bindings(bindings) { }

void UntypedVertexBindingPipelineState::bindAll(const UntypedVertexBindings &bindings, BoundVertexArrayGuard& guard, OpenGLContext& context) {
    for(auto& pipelineBinding : layout) {
        uint32_t index = pipelineBinding.binding;
        const auto& it = bindings.bindings.find(index);
        assert(it != bindings.bindings.end());
        guard.bindVertexBuffer(index, it->second.buffer, it->second.offset, pipelineBinding.stride);
    }
}

UntypedVertexBindingPipelineState::UntypedVertexBindingPipelineState(vector<VertexInputBinding> layout) : layout(layout) {}

void UntypedResourceBindingPipelineState::bindAll(const UntypedResourceBindings &bindings, OpenGLContext& context) {
    for(auto& it : bindings.uniforms) {
        context.bindUniformBuffer(it.second.buffer, it.first, it.second.offset, it.second.size);
    }
    for(auto& it : bindings.textures) {
        context.bindTextureAndSampler(it.first, it.second);
    }
}

UntypedVertexBindingPipelineState UntypedVertexInputCreateInfo::init(VertexArray& array, OpenGLContext& context) {
    context.withBoundVertexArray(array, [this](auto guard) {
        for(auto& attribute : attributes) {
            guard.enableAttribute(attribute.location);
            guard.setAttributeFormat(attribute.location, attribute.format, attribute.offset);
            guard.setAttributeBinding(attribute.location, attribute.binding);
        };

        for(auto& binding : bindings) {
            if(binding.inputRate == InputRate::PER_INSTANCE) {
                guard.setBindingDivisor(binding.binding, 1);
            }
        }
    });

    return UntypedVertexBindingPipelineState {
        .layout = bindings
    };
}

UntypedResourceBindingPipelineState UntypedResourceBindingCreateInfo::init() {
    return UntypedResourceBindingPipelineState();
}

BlendingFunction BlendingFunction::ALPHA = {
    .equation = BlendingEquation::ADD,
    .srcFactor = BlendingFactor::SRC_ALPHA,
    .dstFactor = BlendingFactor::ONE_MINUS_SRC_ALPHA
};

BlendingFunction BlendingFunction::ADDITIVE = {
    .equation = BlendingEquation::ADD,
    .srcFactor = BlendingFactor::ONE,
    .dstFactor = BlendingFactor::ONE
};

bool BlendingFunction::operator==(const BlendingFunction& other) const {
    return equation == other.equation && srcFactor == other.srcFactor && dstFactor == other.dstFactor;
}

optional<Blending> Blending::DISABLED = nullopt;

optional<Blending> Blending::ALPHA = make_optional<Blending>({
    .color = BlendingFunction::ALPHA,
    .alpha = BlendingFunction::ALPHA
});

optional<Blending> Blending::ADDITIVE = make_optional<Blending>({
    .color = BlendingFunction::ADDITIVE,
    .alpha = BlendingFunction::ADDITIVE
});

bool Blending::operator==(const Blending& other) const {
    return color == other.color && alpha == other.alpha;
}

bool ColorBlendPerAttachment::operator==(const ColorBlendPerAttachment& other) const {
    return blending == other.blending && colorMask == other.colorMask;
}

DepthStencilState DepthStencilState::LESS_THAN_OR_EQUAL_TO = {
    .depthTest = make_optional(ComparisonFunction::LESS_THAN_OR_EQUAL_TO),
    .depthWrite = true
};
