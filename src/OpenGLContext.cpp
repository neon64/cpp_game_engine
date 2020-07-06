#include <glad/glad.h>
#include "OpenGLContext.h"
#include "errors.h"
#include "Buffer.h"

#include <iostream>
#include <variant>
#include <memory>
#include <unordered_map>
#include <initializer_list>

using namespace std;

void callVertexAttribFormat(DataFormat format, GLuint location, GLuint offset);

OpenGLContext::OpenGLContext(Window &window) : window(window), defaultRenderTarget(DefaultRenderTarget(window)) {
    window.makeContextCurrent();

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) { exit(-1); }

    printf("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    glDebugMessageCallback(handleGLError, nullptr);
}

OpenGLContext::OpenGLContext(OpenGLContext &&other) : window(other.window), defaultRenderTarget(other.defaultRenderTarget) {}

void OpenGLContext::submit(ClearCommand command) {
    // TODO: unset color, depth and stencil masks
    int bits = 0;
    if(command.color) {
        glClearColor(command.color->r, command.color->g, command.color->b, command.color->a);
        bits |= bits | GL_COLOR_BUFFER_BIT;
    }

    if(command.depth) {
        glClearDepth(*command.depth);
        bits |= GL_DEPTH_BUFFER_BIT;
    }

    if(command.stencil) {
        glClearStencil(*command.stencil);
        bits |= GL_STENCIL_BUFFER_BIT;
    }

    glClear(bits);
}

GLintptr indexFormatGetBytes(IndexFormat format) {
    switch(format) {
        case IndexFormat::UINT16:
            return 16;
        case IndexFormat::UINT32:
            return 32;
        default:
            assert(false);
            return 0;
    }
}

void OpenGLContext::switchPipeline(GraphicsPipeline& pipeline) {
//    if (currentPipeline.has_value() && currentPipeline.value() == pipeline) {
//        return;
//    }
//    currentPipeline = make_optional(pipeline);

    switchProgram(*pipeline.program);

    if (pipeline.depthStencil.depthTest.has_value()) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(pipeline.depthStencil.depthTest.value());
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (pipeline.depthStencil.depthMask != depthMask) {
        glDepthMask(pipeline.depthStencil.depthMask);
        depthMask = !depthMask;
    }

    if(pipeline.inputAssembler.primitiveRestartEnable != primitiveRestartEnabled) {
        if(pipeline.inputAssembler.primitiveRestartEnable) {
            glEnable(GL_PRIMITIVE_RESTART);
            glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        } else {
            glDisable(GL_PRIMITIVE_RESTART);
        }
        primitiveRestartEnabled = !primitiveRestartEnabled;
    }

    if(pipeline.rasterizer.polygonMode != rasterizer.polygonMode) {

        glPolygonMode(GL_FRONT_AND_BACK, pipeline.rasterizer.polygonMode);
        rasterizer.polygonMode = pipeline.rasterizer.polygonMode;
    }

    if(pipeline.rasterizer.culling != rasterizer.culling) {
        if(pipeline.rasterizer.culling.has_value()) {
            glEnable(GL_CULL_FACE);
            glCullFace(pipeline.rasterizer.culling.value());
        } else {
            glDisable(GL_CULL_FACE);
        }
        rasterizer.culling = pipeline.rasterizer.culling;
    }

    if(pipeline.rasterizer.frontFace != rasterizer.frontFace) {
        glFrontFace(pipeline.rasterizer.frontFace);
        rasterizer.frontFace = pipeline.rasterizer.frontFace;
    }

    bindVertexArray(pipeline.vertexArray);
}

void RenderTargetGuard::clear(ClearCommand command) {
    context->submit(command);
}

void RenderTargetGuard::draw(DrawCommand command) {
    context->submit(command);
}

RenderTargetGuard::RenderTargetGuard(OpenGLContext* context) : context(context) {}

void OpenGLContext::submit(DrawCommand command) {
    switchPipeline(command.pipeline);

    for(auto& binding : command.pipeline.vertexInputBindings) {
        uint32_t index = binding.binding;
        const auto& it = command.vertexBuffers.find(index);
        assert(it != command.vertexBuffers.end());
        // TODO: track this state
        glBindVertexBuffer(index, it->second.buffer.getId(), it->second.offset, binding.stride);
    }

    for(auto& it : command.uniformBuffers) {
        bindUniformBuffer(it.second.buffer, it.first, it.second.offset, it.second.size);
    }

    for(auto& it : command.textures) {
        // TODO: track this state
        glActiveTexture(GL_TEXTURE0 + it.first);
        glBindTexture(static_cast<GLuint>(it.second.texture.type), it.second.texture.getId());
        glBindSampler(it.first, it.second.sampler.getId());
    }

    GLenum topology = command.pipeline.inputAssembler.topology;
    if(auto call = std::get_if<NonIndexedDrawCall>(&command.call)) {
        if(command.instanceCount == 1) {
            glDrawArrays(topology, call->firstVertex, call->vertexCount);
        } else {
            if(command.firstInstance == 0) {
                glDrawArraysInstanced(topology, call->firstVertex, call->vertexCount, command.instanceCount);
            }
            glDrawArraysInstancedBaseInstance(topology, call->firstVertex, call->vertexCount, command.instanceCount, command.firstInstance);
        }
    } else if(auto call = std::get_if<IndexedDrawCall>(&command.call)) {
        bindBuffer(*call->indexBuffer.buffer, GL_ELEMENT_ARRAY_BUFFER);

        const void *indexOffset = (const void *) (call->firstIndex * indexFormatGetBytes(call->indexBuffer.format));

        if(command.instanceCount == 1) {
            glDrawElements(topology, call->indexCount, call->indexBuffer.format, indexOffset);
        } else {
            if(command.firstInstance == 0) {
                glDrawElementsInstanced(
                        topology, call->indexCount, call->indexBuffer.format, indexOffset,
                        command.instanceCount);
            } else {
                glDrawElementsInstancedBaseInstance(
                        topology, call->indexCount, call->indexBuffer.format, indexOffset,
                        command.instanceCount, command.firstInstance);
            }

        }
    }
}

GraphicsPipeline OpenGLContext::buildPipeline(GraphicsPipelineCreateInfo info) {
    shared_ptr<Program> program = getProgram(info.shaders);
    VertexArray vertexArray = VertexArray::build();

    bindVertexArray(vertexArray);
    for(auto& attribute : info.vertexInput.attributes) {
        glEnableVertexAttribArray(attribute.location);
        callVertexAttribFormat(attribute.format, attribute.location, attribute.offset);
        glVertexAttribBinding(attribute.location, attribute.binding);
    };

    for(auto& binding : info.vertexInput.bindings) {
        if(binding.inputRate == InputRate::PER_INSTANCE) {
            glVertexBindingDivisor(binding.binding, 1);
        }
    }

    return GraphicsPipeline(
        program,
        std::move(vertexArray),
        info.vertexInput.bindings,
        info.inputAssembler,
        info.rasterizer,
        info.depthStencil,
        info.colorBlend
    );
}

void callVertexAttribFormat(DataFormat format, GLuint location, GLuint offset) {
    switch(format) {
        case R32_SFLOAT:
            glVertexAttribFormat(location, 1, GL_FLOAT, GL_FALSE, offset);
            break;
        case R32G32_SFLOAT:
            glVertexAttribFormat(location, 2, GL_FLOAT, GL_FALSE, offset);
            break;
        case R32G32B32_SFLOAT:
            cout << "calling with " << location << " " << offset << endl;
            glVertexAttribFormat(location, 3, GL_FLOAT, GL_FALSE, offset);
            break;
        case R32G32B32A32_SFLOAT:
            glVertexAttribFormat(location, 4, GL_FLOAT, GL_FALSE, offset);
            break;
        default:
            assert(false);
    }
}

void OpenGLContext::bindVertexArray(VertexArray& vertexArray) {
    if(vertexArray.getId() != currentVertexArray) {
        glBindVertexArray(vertexArray.getId());
        currentVertexArray = vertexArray.getId();
    }
}

Buffer OpenGLContext::buildBuffer(BufferUsage usage, GLsizeiptr size, const void *data, GLbitfield flags) {
    GLuint id;
    glGenBuffers(1, &id);
    Buffer buffer(id, usage, size);

    bindBuffer(buffer, GL_ARRAY_BUFFER);
    glBufferStorage(GL_ARRAY_BUFFER, size, data, flags);

    return std::move(buffer);
}

Buffer OpenGLContext::buildBuffer(BufferUsage usage, GLsizeiptr size, GLbitfield flags) {
    return buildBuffer(usage, size, nullptr, flags);
}

shared_ptr<Program> OpenGLContext::getProgram(ShaderStages stages) {
    auto it = programCache.find(stages);
    if(it != programCache.end()) {
        cout << "found program in cache" << endl;
        return it->second;
    } else {
        cout << "compiling program" << endl;

        // compile new program
        GLuint id = glCreateProgram();

        shared_ptr<Program> program = make_shared<Program>(id);

        program->attachShader(*stages.vertex);
        program->attachShader(*stages.fragment);

        program->linkAndValidate();

        programCache[stages] = program;

        return program;
    }
}

void OpenGLContext::setSwapInterval(int nframes) {
    glfwSwapInterval(nframes);
}

void OpenGLContext::bindBuffer(Buffer &buffer, GLenum target) {
    if(boundBuffers[target] != buffer.getId()) {
        glBindBuffer(target, buffer.getId());
        boundBuffers[target] = buffer.getId();
    }
}

void OpenGLContext::bindUniformBuffer(const Buffer &buffer, GLuint index, GLintptr offset, GLsizeiptr size) {
    //if(boundUniformBuffers[index] != buffer.getId()) {
        glBindBufferRange(GL_UNIFORM_BUFFER, index, buffer.getId(), offset, size);
    //    boundUniformBuffers[index] = buffer.getId();
    //}
}

void OpenGLContext::switchProgram(const Program &program) {
    if(currentProgram != program.getId()) {
        glUseProgram(program.getId());
        currentProgram = program.getId();
    }
}

void OpenGLContext::bindTexture(Texture &texture) {
    glBindTexture(static_cast<GLuint>(texture.type), texture.getId());
}

GLuint getTextureFormat(DataFormat format) {
    switch(format) {
        case R8G8B8_UINT:
            return GL_RGB8;
        case R8G8B8A8_UINT:
            return GL_RGBA8;
        case R8G8B8_SRGB:
            return GL_SRGB8;
        case R8G8B8A8_SRGB:
            return GL_SRGB8_ALPHA8;
        case D16_UNORM:
            return GL_DEPTH_COMPONENT16;
        case D24_UNORM:
            return GL_DEPTH_COMPONENT24;
        case D32_SFLOAT:
            return GL_DEPTH_COMPONENT32F;
        default:
            assert(false);
            return 0;
    }
}

GLuint getTransferDataFormat(TransferFormat format) {
    switch(format) {
        case TransferFormat::R8G8B8_UINT:
            return GL_RGB;
        default:
            assert(false);
            return 0;
    }
}

GLuint getTransferDataType(TransferFormat format) {
    switch(format) {
        case TransferFormat::R8G8B8_UINT:
            return GL_UNSIGNED_BYTE;
        default:
            assert(false);
            return 0;
    }
}

Texture OpenGLContext::buildTexture2D(DataFormat format, Dimensions2d size, bool hasMipMaps) {
    TextureType type = TextureType::TEXTURE_2D;
    GLuint id;
    glGenTextures(1, &id);
    Texture tex(id, type, format, size, hasMipMaps);

    bindTexture(tex);
    glTexStorage2D(static_cast<GLuint>(type), hasMipMaps ? floor(log2(max(size.width, size.height))) + 1 : 1, getTextureFormat(format), size.width, size.height);

    return std::move(tex);
}

void OpenGLContext::uploadBaseImage2D(Texture& texture, TransferFormat format, const void *data) {
    bindTexture(texture);
    glTexSubImage2D(static_cast<GLuint>(texture.type), 0, 0, 0, texture.size.width, texture.size.height, getTransferDataFormat(format), getTransferDataType(format), data);
    if(texture.hasMipMaps) {
        glGenerateMipmap(static_cast<GLuint>(texture.type));
    }
}

DefaultRenderTarget &OpenGLContext::getDefaultRenderTarget() {
    return defaultRenderTarget;
}

void OpenGLContext::bindFramebuffer(GLuint id) {
    if(currentFramebuffer != id) {
        glBindFramebuffer(GL_FRAMEBUFFER, id);
        currentFramebuffer = id;
    }
}

Renderbuffer OpenGLContext::buildRenderbuffer(Dimensions2d size, RenderbufferInternalFormat format) {
    GLuint id;
    glGenRenderbuffers(1, &id);
    Renderbuffer renderbuffer(id, format, size);
    bindRenderbuffer(renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLuint>(format), size.width, size.height);
    return std::move(renderbuffer);
}

void OpenGLContext::bindRenderbuffer(Renderbuffer &renderbuffer) {
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer.getId());
}

Framebuffer OpenGLContext::buildFramebuffer(ColorAttachments colorAttachments, MaybeDepthAttachment depthAttachment, MaybeStencilAttachment stencilAttachment) {
    GLuint id;
    glGenFramebuffers(1, &id);
    bindFramebuffer(id);
    Dimensions2d minSize(INT_MAX, INT_MAX);
    for(auto& it : colorAttachments) {
        it.second->attach(it.first);
        minSize = minSize.min(it.second->getSize());
    }
    if(depthAttachment.has_value()) {
        depthAttachment.value()->attach();
        minSize = minSize.min(depthAttachment.value()->getSize());
    }
    if(stencilAttachment.has_value()) {
        stencilAttachment.value()->attach();
        minSize = minSize.min(stencilAttachment.value()->getSize());
    }

    return Framebuffer(id, minSize, std::move(colorAttachments), std::move(depthAttachment), std::move(stencilAttachment));
}

void OpenGLContext::bindReadFramebuffer(GLuint framebufferId) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferId);
}
