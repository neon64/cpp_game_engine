#include <glad/glad.h>
#include "OpenGLContext.h"
#include "errors.h"
#include "buffer.h"

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

    bindBuffer(buffer, GL_ARRAY_BUFFER);
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

void OpenGLContext::bindBuffer(UntypedBuffer &buffer, GLenum target) {
    if(boundBuffers[target] != buffer.getId()) {
        glBindBuffer(target, buffer.getId());
        boundBuffers[target] = buffer.getId();
    }
}

void OpenGLContext::bindUniformBuffer(const UntypedBuffer &buffer, GLuint index, GLintptr offset, GLsizeiptr size) {
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
                                    GLuint instanceCount, GLuint firstInstance) {
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
        bindBuffer(*call->indexBuffer.buffer, GL_ELEMENT_ARRAY_BUFFER);

        const void *indexOffset = (const void *) (call->firstIndex * indexFormatGetBytes(call->indexBuffer.format));

        if(instanceCount == 1) {
            glDrawElements(top, call->indexCount, call->indexBuffer.format, indexOffset);
        } else {
            if(firstInstance == 0 && call->firstVertex == 0) {
                glDrawElementsInstanced(
                        top, call->indexCount, call->indexBuffer.format, indexOffset,
                        instanceCount);
            } else {
                glDrawElementsInstancedBaseVertexBaseInstance(
                        top, call->indexCount, call->indexBuffer.format, indexOffset,
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

void OpenGLContext::enableVertexArrayAttribute(VertexArray &array, GLuint location) {
    bindVertexArray(array);
    glEnableVertexAttribArray(location);
}

void
OpenGLContext::setVertexArrayAttributeFormat(VertexArray &array, GLuint location, DataFormat format, GLuint offset) {
    bindVertexArray(array);
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

void OpenGLContext::setVertexArrayAttributeBinding(VertexArray &array, GLuint location, GLuint binding) {
    bindVertexArray(array);
    glVertexAttribBinding(location, binding);
}

void OpenGLContext::setVertexArrayBindingDivisor(VertexArray &array, GLuint binding, GLuint divisor) {
    bindVertexArray(array);
    glVertexBindingDivisor(binding, divisor);
}

void OpenGLContext::bindVertexBuffer(VertexArray &array, GLuint index, const UntypedBuffer &buffer, uint32_t offset, uint32_t stride) {
    bindVertexArray(array);
    // TODO: track this state
    glBindVertexBuffer(index, buffer.getId(), offset, stride);
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
        glBlendColor(newColorBlend.constants.x, newColorBlend.constants.y, newColorBlend.constants.z, newColorBlend.constants.w);
        newColorBlend.constants= colorBlend.constants;
    }
}
