
#ifndef GAME_ENGINE_VERTEXARRAY_H
#define GAME_ENGINE_VERTEXARRAY_H

#include <memory>
#include <glad/glad.h>
#include "OpenGLResource.h"

class VertexArray : public OpenGLResource<VertexArray> {
public:
    explicit VertexArray(GLuint id);

    void destroyResource();

    static std::shared_ptr<VertexArray> build();
};


#endif //GAME_ENGINE_VERTEXARRAY_H
