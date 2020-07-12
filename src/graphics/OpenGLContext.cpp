#include <glad/glad.h>
#include "OpenGLContext.h"
#include "../errors.h"
#include "buffer.h"

#define LOGURU_WITH_STREAMS 1
#include <loguru/loguru.hpp>

#include <iostream>
#include <variant>
#include <memory>
#include <unordered_map>
#include <initializer_list>

using namespace std;


OpenGLContext::OpenGLContext(Window &window) : window(window), defaultRenderTarget(DefaultRenderTarget(window)) {
    window.makeContextCurrent();

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) { exit(-1); }

    printf("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    glDebugMessageCallback(handleGLError, nullptr);
}

OpenGLContext::OpenGLContext(OpenGLContext &&other) : window(other.window), defaultRenderTarget(other.defaultRenderTarget) {}

void OpenGLContext::submit(ClearCommand command) {
    int bits = 0;
    if(command.color) {
        glClearColor(command.color->r, command.color->g, command.color->b, command.color->a);
        bits |= bits | GL_COLOR_BUFFER_BIT;

        for(auto& attachment : colorBlend.attachments) {
            auto writeAll = tuple(true, true, true, true);
            if(attachment.second.colorMask != writeAll) {
                glColorMaski(attachment.first, true, true, true, true);
                attachment.second.colorMask = writeAll;
            }
        }
    }

    if(command.depth) {
        glClearDepth(*command.depth);
        bits |= GL_DEPTH_BUFFER_BIT;

        if(!depthStencil.depthWrite) {
            glDepthMask(GL_TRUE);
            depthStencil.depthWrite = true;
        }
    }

    if(command.stencil) {
        glClearStencil(*command.stencil);
        bits |= GL_STENCIL_BUFFER_BIT;

        // TODO: unset stencil mask
    }

    glClear(bits);
}

void RenderTargetGuard::clear(ClearCommand command) {
    context->submit(command);
}

RenderTargetGuard::RenderTargetGuard(OpenGLContext* context) : context(context) {}



void OpenGLContext::bindVertexArray(VertexArray& vertexArray) {
    if(vertexArray.getId() != currentVertexArray) {
        glBindVertexArray(vertexArray.getId());
        currentVertexArray = vertexArray.getId();
    }
}

UntypedBuffer OpenGLContext::buildBuffer(BufferUsage usage, GLsizeiptr size, const void *data, GLbitfield flags) {
    GLuint id;
    glGenBuffers(1, &id);
    UntypedBuffer buffer(id, usage, size);

    bindArrayBuffer(buffer);
    glBufferStorage(GL_ARRAY_BUFFER, size, data, flags);

    return std::move(buffer);
}

UntypedBuffer OpenGLContext::buildBuffer(BufferUsage usage, GLsizeiptr size, GLbitfield flags) {
    return buildBuffer(usage, size, nullptr, flags);
}


shared_ptr<Program> OpenGLContext::getProgram(ShaderStages stages) {
    auto it = programCache.find(stages);
    if(it != programCache.end()) {
        cout << "found program in cache" << endl;
        return it->second;
    } else {
        // compile new program
        GLuint id = glCreateProgram();

        shared_ptr<Program> program = make_shared<Program>(id);

        program->attachShader(*stages.vertex);
        program->attachShader(*stages.fragment);

        cout << "linking and validating program (id = " << id << ")" << endl;

        program->linkAndValidate();

        programCache[stages] = program;

        return program;
    }
}

void OpenGLContext::setSwapInterval(int nframes) {
    glfwSwapInterval(nframes);
}

void OpenGLContext::bindArrayBuffer(const UntypedBuffer &buffer) {
    if(boundArrayBuffer != buffer.getId()) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer.getId());
        boundArrayBuffer = buffer.getId();
    }
}

void OpenGLContext::bindUniformBuffer(const UntypedBuffer &buffer, GLuint index, GLintptr byteOffset, GLsizeiptr size) {
    CurrentUniformBufferBinding b {
        .bufferId = buffer.getId(),
        .byteOffset = byteOffset,
        .size = size
    };

    auto it = boundUniformBuffers.find(index);
    if(it == boundUniformBuffers.end() || it->second != b) {
        LOG_S(INFO) << "binding buffer range index=" << index << " id=" << buffer.getId() << " offset=" << byteOffset << " size=" << size;
        glBindBufferRange(GL_UNIFORM_BUFFER, index, buffer.getId(), byteOffset, size);

        boundUniformBuffers[index] = b;
    }
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
        case TransferFormat::R8G8B8A8_UINT:
            return GL_RGBA;
        default:
            assert(false);
            return 0;
    }
}

GLuint getTransferDataType(TransferFormat format) {
    switch(format) {
        case TransferFormat::R8G8B8_UINT:
        case TransferFormat::R8G8B8A8_UINT:
            return GL_UNSIGNED_BYTE;
        default:
            assert(false);
            return 0;
    }
}

Texture2d OpenGLContext::buildTexture2D(DataFormat format, Dimensions2d size, bool hasMipMaps) {
    GLuint id;
    glGenTextures(1, &id);
    Texture2d tex(id, format, size, hasMipMaps);

    bindTexture(tex);
    glTexStorage2D(static_cast<GLuint>(tex.type), hasMipMaps ? floor(log2(max(size.width, size.height))) + 1 : 1, getTextureFormat(format), size.width, size.height);

    return std::move(tex);
}

void OpenGLContext::uploadBaseImage2D(Texture2d& texture, TransferFormat format, const void *data) {
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

void OpenGLContext::performDrawCall(PrimitiveTopology topology, variant<NonIndexedDrawCall, IndexedDrawCall> drawCall,
                                    GLuint instanceCount, GLuint firstInstance, BoundVertexArrayGuard guard) {
    GLenum top = static_cast<GLenum>(topology);
    if(auto call = std::get_if<NonIndexedDrawCall>(&drawCall)) {
        if(instanceCount == 1) {
            glDrawArrays(top, call->firstVertex, call->vertexCount);
        } else {
            if(firstInstance == 0 && call->firstVertex == 0) {
                glDrawArraysInstanced(top, call->firstVertex, call->vertexCount, instanceCount);
            } else {
                glDrawArraysInstancedBaseInstance(top, call->firstVertex, call->vertexCount,
                                                  instanceCount,
                                                  firstInstance);
            }
        }
    } else if(auto call = std::get_if<IndexedDrawCall>(&drawCall)) {
        guard.bindIndexBuffer(call->indexBuffer.buffer);

        GLintptr byteOffset = call->indexBuffer.byteOffset;

        if(instanceCount == 1 && call->firstVertex == 0 && firstInstance == 0) {
            glDrawElements(top, call->indexBuffer.indexCount, static_cast<GLenum>(call->indexBuffer.format), (const void *) byteOffset);
        } else {
            if(firstInstance == 0 && call->firstVertex == 0) {
                glDrawElementsInstanced(
                        top, call->indexBuffer.indexCount, static_cast<GLenum>(call->indexBuffer.format), (const void *) byteOffset,
                        instanceCount);
            } else {
                glDrawElementsInstancedBaseVertexBaseInstance(
                        top, call->indexBuffer.indexCount, static_cast<GLenum>(call->indexBuffer.format), (const void *) byteOffset,
                        instanceCount, call->firstVertex, firstInstance);
            }
        }
    }
}

void OpenGLContext::bindTextureAndSampler(uint32_t unit, const Texture &texture, const Sampler &sampler) {
    // TODO: track this state
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(static_cast<GLuint>(texture.type), texture.getId());
    glBindSampler(unit, sampler.getId());
}

void OpenGLContext::switchRasterizerState(RasterizerState &newRasterizer) {
    if(newRasterizer.polygonMode != rasterizer.polygonMode) {
        glPolygonMode(GL_FRONT_AND_BACK, newRasterizer.polygonMode);
        rasterizer.polygonMode = newRasterizer.polygonMode;
    }
    if(newRasterizer.culling != rasterizer.culling) {
        if(newRasterizer.culling.has_value()) {
            glEnable(GL_CULL_FACE);
            glCullFace(newRasterizer.culling.value());
        } else {
            glDisable(GL_CULL_FACE);
        }
        rasterizer.culling = newRasterizer.culling;
    }
    if(newRasterizer.frontFace != rasterizer.frontFace) {
        glFrontFace(newRasterizer.frontFace);
        rasterizer.frontFace = newRasterizer.frontFace;
    }
}

void OpenGLContext::switchDepthStencilState(DepthStencilState &newDepthStencil) {
    if(newDepthStencil.depthTest != depthStencil.depthTest) {
        if(newDepthStencil.depthTest.has_value()) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(newDepthStencil.depthTest.value());
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        depthStencil.depthTest = newDepthStencil.depthTest;
    }
    if(newDepthStencil.depthWrite != depthStencil.depthWrite) {
        glDepthMask(newDepthStencil.depthWrite);
        depthStencil.depthWrite = newDepthStencil.depthWrite;
    }
}

void OpenGLContext::switchInputAssemblerState(InputAssemblerState &newInputAssembler) {
    // input assembler
    if(newInputAssembler.primitiveRestartEnable != primitiveRestartEnabled) {
        if(newInputAssembler.primitiveRestartEnable) {
            glEnable(GL_PRIMITIVE_RESTART);
            glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        } else {
            glDisable(GL_PRIMITIVE_RESTART);
        }
        primitiveRestartEnabled = !primitiveRestartEnabled;
    }
}

void OpenGLContext::switchColorBlendState(ColorBlendState &newColorBlend) {
    for(auto& attachment : newColorBlend.attachments) {
        auto existing = colorBlend.attachments.find(attachment.first);
        if(existing == colorBlend.attachments.end() || existing->second != attachment.second) {
            if(attachment.second.blending.has_value()) {
                glEnablei(GL_BLEND, attachment.first);
                glBlendFuncSeparatei(attachment.first, attachment.second.blending->color.srcFactor, attachment.second.blending->color.dstFactor, attachment.second.blending->alpha.srcFactor, attachment.second.blending->alpha.dstFactor);
                glBlendEquationSeparatei(attachment.first, attachment.second.blending->color.equation, attachment.second.blending->alpha.equation);
            } else {
                glDisablei(GL_BLEND, attachment.first);
            }

            glColorMaski(attachment.first, get<0>(attachment.second.colorMask), get<1>(attachment.second.colorMask), get<2>(attachment.second.colorMask), get<3>( attachment.second.colorMask));

            colorBlend.attachments[attachment.first] = attachment.second;
        }
    }
    if(newColorBlend.constants != colorBlend.constants) {
        glBlendColor(newColorBlend.constants.r, newColorBlend.constants.g, newColorBlend.constants.b, newColorBlend.constants.a);
        newColorBlend.constants= colorBlend.constants;
    }
}

Shader OpenGLContext::buildShader(ShaderType type, std::string description, const string_view &source) {
    GLuint id = glCreateShader(type);
    glObjectLabel(GL_SHADER, id, description.size(), description.c_str());

    Shader s(id, type, description);
    s.compile(source);

    return s;
}

BoundVertexArrayGuard::BoundVertexArrayGuard(VertexArray &array) : array(array) {

}

void BoundVertexArrayGuard::enableAttribute(GLuint location) {
    glEnableVertexAttribArray(location);
}

void BoundVertexArrayGuard::setAttributeFormat(GLuint location, DataFormat format, GLuint offset) {
    switch(format) {
        case R32_SFLOAT:
            glVertexAttribFormat(location, 1, GL_FLOAT, GL_FALSE, offset);
            break;
        case R32G32_SFLOAT:
            glVertexAttribFormat(location, 2, GL_FLOAT, GL_FALSE, offset);
            break;
        case R32G32B32_SFLOAT:
            glVertexAttribFormat(location, 3, GL_FLOAT, GL_FALSE, offset);
            break;
        case R32G32B32A32_SFLOAT:
            glVertexAttribFormat(location, 4, GL_FLOAT, GL_FALSE, offset);
            break;
        default:
            assert(false);
    }
}

void BoundVertexArrayGuard::setAttributeBinding(GLuint location, GLuint binding) {
    glVertexAttribBinding(location, binding);
}

void BoundVertexArrayGuard::setBindingDivisor(GLuint binding, GLuint divisor) {
    glVertexBindingDivisor(binding, divisor);
}

void BoundVertexArrayGuard::bindVertexBuffer(GLuint binding, const UntypedBuffer &buffer, uint32_t byteOffset, uint32_t stride) {
    CurrentVertexBufferBinding b {
        .bufferId = buffer.getId(),
        .offset = byteOffset
    };
    auto it = array.currentVertexBufferBindings.find(binding);
    if(it == array.currentVertexBufferBindings.end() || it->second != b) {
        LOG_S(INFO) << "binding vertex buffer id=" << buffer.getId() << " offset=" << byteOffset << " stride=" << stride;
        assert(byteOffset < buffer.size);
        glBindVertexBuffer(binding, buffer.getId(), byteOffset, stride);
        array.currentVertexBufferBindings[binding] = b;
    }
}

void BoundVertexArrayGuard::bindIndexBuffer(const UntypedBuffer &buffer) {
    if(array.currentElementArrayBuffer != buffer.getId()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.getId());
        array.currentElementArrayBuffer = buffer.getId();
    }
}
