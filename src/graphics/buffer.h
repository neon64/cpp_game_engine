
#ifndef GAME_ENGINE_BUFFER_H
#define GAME_ENGINE_BUFFER_H

#include "OpenGLResource.h"
#include <memory>
#include <span>
#include <cassert>

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
    UntypedBuffer(GLuint id, BufferUsage usage, size_t size);

    const size_t size;
    const BufferUsage usage;

    UntypedBuffer* onHeap() {
        return new UntypedBuffer(std::move(*this));
    };

    void destroyResource();
};

#define accessField(type, fieldname) slice<type>([](auto buffer) { return &buffer->fieldname; })

template<typename T>
struct BufferView {
    UntypedBuffer& buffer;
    const size_t byteOffset;

    BufferView(UntypedBuffer& buffer) : buffer(buffer), byteOffset(0) {
        assert(sizeof(T) == buffer.size);
    }

    BufferView(UntypedBuffer& buffer, size_t byteOffset) : buffer(buffer), byteOffset(byteOffset) {
        assert(byteOffset + sizeof(T) <= buffer.size);
    }

    size_t getSize() const {
        return sizeof(T);
    }

    // borrowing ideas from the `slice_custom` method from the Rust 'Vulkano' library.
    // https://docs.rs/vulkano/0.19.0/vulkano/buffer/struct.BufferSlice.html
    //
    // Same caveats apply:
    // The object whose reference is passed to the closure is uninitialized. Therefore you must not access the content of the object.
    // You must return a reference to an element from the parameter. The closure must not panic.
    template<typename T2, typename F>
    BufferView<T2> slice(F callback) {
        T* t = (T*) 0;
        T2 *result = callback(t);
        size_t offset = (size_t) result;
        size_t size = sizeof(T2);

        assert(offset <= sizeof(T));
        assert(offset + size <= sizeof(T));

        return BufferView<T2>(buffer, offset);
    }

    T* convert(void *ptr) const {
        assert(ptr != nullptr);
        return static_cast<T*>(ptr);
    }
};

template<typename T>
class Buffer {
    UntypedBuffer inner;
public:
    Buffer(UntypedBuffer&& inner) : inner(std::move(inner)) {}

    BufferView<T> getView() {
        return BufferView<T>(inner);
    }

    Buffer<T>* onHeap() {
        return new Buffer<T>(std::move(*this));
    };
};

struct Slice {
    size_t elementOffset;
    size_t numElements;
};

template<typename T>
struct BufferSlice {
    UntypedBuffer& buffer;
    const size_t byteOffset;
    const size_t numElements;

    BufferSlice(UntypedBuffer& buffer) : buffer(buffer), byteOffset(0), numElements(buffer.size / sizeof(T)) {
        assert(buffer.size % sizeof(T) == 0);
    }

    BufferSlice(UntypedBuffer& buffer, size_t byteOffset, size_t numElements) : buffer(buffer), byteOffset(byteOffset), numElements(numElements) {
        assert((getSize() + byteOffset) <= buffer.size);
    }

    BufferSlice(UntypedBuffer& buffer, size_t numElements) : BufferSlice(buffer, 0, numElements) {}

    BufferSlice<T> subslice(Slice slice) {
        assert(slice.numElements <= numElements);
        size_t newOffset = byteOffset + slice.elementOffset * sizeof(T);
        assert(newOffset + slice.numElements * sizeof(T) <= buffer.size);
        return BufferSlice<T>(buffer, newOffset, slice.numElements);
    }

    size_t getSize() const {
        return sizeof(T) * numElements;
    }

    span<T> convert(void *ptr) const {
        return span(static_cast<T*>(ptr), numElements);
    }
};

template<typename T>
class ArrayBuffer {
    UntypedBuffer inner;

public:
    ArrayBuffer(UntypedBuffer&& inner) : inner(std::move(inner)) {}

    BufferSlice<T> getSlice() {
        return BufferSlice<T>(inner, inner.size / sizeof(T));
    }

    ArrayBuffer<T>* onHeap() {
        return new ArrayBuffer<T>(std::move(*this));
    };

    UntypedBuffer& unsafeGetInner() {
        return inner;
    }
};

struct UntypedBufferBinding {
    const UntypedBuffer& buffer;
    const size_t offset;
    const size_t size;
};

template<typename T>
struct VertexBufferBinding {
    const UntypedBuffer& buffer;
    const size_t byteOffset;

    VertexBufferBinding(UntypedBuffer& buffer, size_t offset) : buffer(buffer), byteOffset(offset) {}
    VertexBufferBinding(BufferSlice<T> slice) : buffer(slice.buffer), byteOffset(slice.byteOffset) {}
};

struct UntypedVertexBufferBinding {
    const UntypedBuffer& buffer;
    const size_t offset;
};

enum class IndexFormat {
    UINT16 = GL_UNSIGNED_SHORT,
    UINT32 = GL_UNSIGNED_INT
};

size_t indexFormatGetBytes(IndexFormat format);

struct IndexBufferBinding {
    const UntypedBuffer& buffer;
    const IndexFormat format;
    const size_t indexCount;
    const size_t byteOffset = 0;

    IndexBufferBinding(BufferSlice<uint32_t> slice) : buffer(slice.buffer), format(IndexFormat::UINT32), indexCount(slice.numElements), byteOffset(slice.byteOffset) {}
    IndexBufferBinding(BufferSlice<uint16_t> slice) : buffer(slice.buffer), format(IndexFormat::UINT16), indexCount(slice.numElements), byteOffset(slice.byteOffset) {}
};


#endif //GAME_ENGINE_BUFFER_H
