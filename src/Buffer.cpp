
#include "Buffer.h"
#include <memory>

using namespace std;

Buffer::Buffer(GLuint id, BufferUsage usage) : OpenGLResource(id), usage(usage) {

}

void Buffer::destroyResource() {
    glDeleteBuffers(1, &id);
}

shared_ptr<Buffer> Buffer::build(BufferUsage usage) {
    GLuint id;
    glGenBuffers(1, &id);
    return make_shared<Buffer>(id, usage);
}
