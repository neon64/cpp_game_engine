#include <glad/glad.h>
#include "OpenGLContext.h"
#include "errors.h"

#include <iostream>
#include <memory>

using namespace std;

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

void OpenGLContext::submit(DrawCommand command) {
    // cout << "submitting drawing command" << endl;
}

shared_ptr<GraphicsPipeline> OpenGLContext::buildPipeline(GraphicsPipelineCreateInfo info) {
    return make_shared<GraphicsPipeline>();
}

void OpenGLContext::bindVertexArray(VertexArray& vertexArray) {
    if(vertexArray.getId() != currentVertexArray) {
        glBindVertexArray(vertexArray.getId());
        currentVertexArray = vertexArray.getId();
    }
}