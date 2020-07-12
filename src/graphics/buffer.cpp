
#include "buffer.h"
#include <memory>

using namespace std;

UntypedBuffer::UntypedBuffer(GLuint id, BufferUsage usage, size_t size) : OpenGLResource(id), usage(usage), size(size) {}

void UntypedBuffer::destroyResource() {
    glDeleteBuffers(1, &id);
}

size_t indexFormatGetBytes(IndexFormat format) {
    switch(format) {
        case IndexFormat::UINT16:
            return sizeof(GLushort);
        case IndexFormat::UINT32:
            return sizeof(GLuint);
        default:
            assert(false);
            return 0;
    }
}