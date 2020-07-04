#include <glad/glad.h>
#include "OpenGLContext.h"
#include "errors.h"
#include "Buffer.h"

#include <iostream>
#include <variant>
#include <memory>

using namespace std;

void callVertexAttribFormat(DataFormat format, GLuint location, GLuint offset);

OpenGLContext::OpenGLContext(Window &window) {
    window.makeContextCurrent();

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) { exit(-1); }

    printf("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

    glDebugMessageCallback(handleGLError, nullptr);
}

OpenGLContext::OpenGLContext(OpenGLContext &&other) {}

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
    }
}

void OpenGLContext::switchPipeline(const shared_ptr<GraphicsPipeline>& pipeline) {
    if (currentPipeline.has_value() && currentPipeline.value() == pipeline) {
        return;
    }
    currentPipeline = make_optional(pipeline);

    switchProgram(*pipeline->program);

    if (pipeline->depthStencil.depthTest.has_value()) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(pipeline->depthStencil.depthTest.value());
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (pipeline->depthStencil.depthMask != depthMask) {
        if(pipeline->depthStencil.depthMask) {
            glDepthMask(true);
        } else {
            glDepthMask(false);
        }
        depthMask = !depthMask;
    }

    if(pipeline->inputAssembler.primitiveRestartEnable != primitiveRestartEnabled) {
        if(pipeline->inputAssembler.primitiveRestartEnable) {
            glEnable(GL_PRIMITIVE_RESTART);
            glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        } else {
            glDisable(GL_PRIMITIVE_RESTART);
        }
        primitiveRestartEnabled = !primitiveRestartEnabled;
    }

    if(pipeline->rasterizer.culling != rasterizer.culling) {
        if(pipeline->rasterizer.culling.has_value()) {
            glEnable(GL_CULL_FACE);
            glCullFace(pipeline->rasterizer.culling.value());
        } else {
            glDisable(GL_CULL_FACE);
        }
        rasterizer.culling = pipeline->rasterizer.culling;
    }

    if(pipeline->rasterizer.frontFace != rasterizer.frontFace) {
        glFrontFace(pipeline->rasterizer.frontFace);
        rasterizer.frontFace = pipeline->rasterizer.frontFace;
    }

    bindVertexArray(*pipeline->vertexArray);
}

void OpenGLContext::submit(DrawCommand command) {
    switchPipeline(command.pipeline);

    for(auto& binding : command.pipeline->vertexInputBindings) {
        uint32_t index = binding.binding;
        const auto& it = command.vertexBuffers.find(index);
        assert(it != command.vertexBuffers.end());
        // TODO: state track this
        glBindVertexBuffer(index, it->second.buffer->getId(), it->second.offset, binding.stride);
    }

    for(auto& it : command.uniformBuffers) {
        bindUniformBuffer(*it.second.buffer, it.first, it.second.offset, it.second.size);
    }

    GLenum topology = command.pipeline->inputAssembler.topology;
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

shared_ptr<GraphicsPipeline> OpenGLContext::buildPipeline(GraphicsPipelineCreateInfo info) {
    shared_ptr<Program> program = getProgram(info.shaders);
    shared_ptr<VertexArray> vertexArray = VertexArray::build();

    bindVertexArray(*vertexArray);
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

    return make_shared<GraphicsPipeline>(
        program,
        vertexArray,
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
    }
}

void OpenGLContext::bindVertexArray(VertexArray& vertexArray) {
    if(vertexArray.getId() != currentVertexArray) {
        glBindVertexArray(vertexArray.getId());
        currentVertexArray = vertexArray.getId();
    }
}

void OpenGLContext::setBufferStorage(Buffer& buffer, GLsizeiptr size, const void *data, GLbitfield flags) {
    bindBuffer(buffer, GL_ARRAY_BUFFER);

    glBufferStorage(GL_ARRAY_BUFFER, size, data, flags);
    buffer.size = size;
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

void OpenGLContext::bindUniformBuffer(Buffer &buffer, GLuint index, GLintptr offset, GLsizeiptr size) {
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
