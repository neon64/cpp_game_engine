
#ifndef GAME_ENGINE_VERTEXARRAY_H
#define GAME_ENGINE_VERTEXARRAY_H

#include <memory>
#include <glad/glad.h>
#include <unordered_map>

#include "OpenGLResource.h"

class BoundVertexArrayGuard;

struct CurrentVertexBufferBinding {
    GLuint bufferId;
    uint32_t offset;
    uint32_t stride;

    bool operator==(const CurrentVertexBufferBinding& other) {
        return bufferId == other.bufferId && offset == other.offset && stride == other.stride;
    }
};

class VertexArray : public OpenGLResource<VertexArray> {
    GLuint currentElementArrayBuffer = 0;
    std::unordered_map<GLuint, CurrentVertexBufferBinding> currentVertexBufferBindings;

    friend class BoundVertexArrayGuard;

public:
    explicit VertexArray(GLuint id);

    void destroyResource();

    static VertexArray build();
};


#endif //GAME_ENGINE_VERTEXARRAY_H
