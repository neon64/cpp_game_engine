
#ifndef GAME_ENGINE_BUFFER_H
#define GAME_ENGINE_BUFFER_H

#include "OpenGLResource.h"
#include <memory>

using namespace std;

enum BufferUsage {
    STATIC_DRAW  = GL_STATIC_DRAW,
    DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
    STREAM_DRAW = GL_STREAM_DRAW
};

class Buffer : public OpenGLResource<Buffer> {
public:
    Buffer(GLuint id, BufferUsage usage, GLintptr size);

    const GLintptr size;
    const BufferUsage usage;

    void destroyResource();
};

enum IndexFormat {
    UINT16 = GL_UNSIGNED_SHORT,
    UINT32 = GL_UNSIGNED_INT
};

struct IndexBufferRef {
    const shared_ptr<Buffer> buffer;
    const IndexFormat format;
};


#endif //GAME_ENGINE_BUFFER_H
