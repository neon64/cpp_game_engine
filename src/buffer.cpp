
#include "buffer.h"
#include <memory>

using namespace std;

UntypedBuffer::UntypedBuffer(GLuint id, BufferUsage usage, GLintptr size) : OpenGLResource(id), usage(usage), size(size) {}

void UntypedBuffer::destroyResource() {
    glDeleteBuffers(1, &id);
}
