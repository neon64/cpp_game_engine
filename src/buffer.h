
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

enum DataFormat {
    R8G8B8_UINT,
    R8G8B8A8_UINT,
    R8G8B8_SRGB,
    R8G8B8A8_SRGB,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    R32G32B32A32_SFLOAT,
    D16_UNORM,
    D24_UNORM,
    D32_SFLOAT
};

class UntypedBuffer : public OpenGLResource<UntypedBuffer> {
public:
    UntypedBuffer(GLuint id, BufferUsage usage, GLintptr size);

    const GLintptr size;
    const BufferUsage usage;

    void destroyResource();
};

//template<typename T>
//class ArrayBuffer : public UntypedBuffer {
//    Buffer(GLuint id, BufferUsage usage, uintptr numElements);
//};
//
//template<typename T>
//ArrayBuffer<T>::Buffer(GLuint id, BufferUsage usage, uintptr numElements) : Buffer(id, usage, sizeof(T) * numElements) { }

template<typename T>
struct VertexBufferBinding {
    const UntypedBuffer& buffer;
    const uint32_t offset;
};

struct UntypedVertexBufferBinding {
    const UntypedBuffer& buffer;
    const uint32_t offset;
};

template<typename T>
struct UniformBufferBinding {
    const UntypedBuffer& buffer;
    const uint32_t offset;

    uint32_t getSize() {
        return sizeof(T);
    }
};

struct UntypedUniformBufferBinding {
    const UntypedBuffer& buffer;
    const uint32_t offset;
    const uint32_t size;
};

enum IndexFormat {
    UINT16 = GL_UNSIGNED_SHORT,
    UINT32 = GL_UNSIGNED_INT
};

struct IndexBufferRef {
    const shared_ptr<UntypedBuffer> buffer;
    const IndexFormat format;
};


#endif //GAME_ENGINE_BUFFER_H
