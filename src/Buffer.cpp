
#include "Buffer.h"
#include <memory>

using namespace std;

Buffer::Buffer(GLuint id, BufferUsage usage, GLintptr size) : OpenGLResource(id), usage(usage), size(size) {}

void Buffer::destroyResource() {
    glDeleteBuffers(1, &id);
}

UniformBufferBinding Buffer::asUniformBuffer() const {
    return UniformBufferBinding {
        .buffer = *this,
        .offset = 0,
        .size = size
    };
}
