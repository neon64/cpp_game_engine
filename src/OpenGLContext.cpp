#include <glad/glad.h>
#include "OpenGLContext.h"
#include "errors.h"

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
