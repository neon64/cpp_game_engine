
#ifndef GAME_ENGINE_OPENGLRESOURCE_H
#define GAME_ENGINE_OPENGLRESOURCE_H

#include <glad/glad.h>
#include <memory>
#include <iostream>

template<typename T>
class OpenGLResource {
protected:
    GLuint id;
public:
    OpenGLResource(GLuint id) : id(id) { }

    GLuint getId() const { return id; }

    OpenGLResource(const OpenGLResource&) = delete;
    OpenGLResource& operator=(const OpenGLResource&) = delete;
    OpenGLResource(OpenGLResource&& other) : id(std::move(other.id)) {
        std::cout << "setting source id to zero" << std::endl;
        other.id = 0;
    }

    ~OpenGLResource() {
        if(id == 0) { return; }
        ((T *) this)->destroyResource();
    }
};

#endif //GAME_ENGINE_OPENGLRESOURCE_H
