
#include <glad/glad.h>
#include <cassert>
#include <iostream>

/*
 * from https://gist.github.com/nlguillemot/b1981969b07295376674
 */
const char* gl_error_to_string(GLenum err) {
    switch (err) {
        case GL_NO_ERROR:                      return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
        case GL_STACK_OVERFLOW:                return "GL_STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW:               return "GL_STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
        case 0x8031: /* not core */            return "GL_TABLE_TOO_LARGE_EXT";
        case 0x8065: /* not core */            return "GL_TEXTURE_TOO_LARGE_EXT";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default:
            assert(!"Unhandled GL error code");
            return NULL;
    }
}

void handleGLFWError(int code, const char* desc) {
    printf("GLFW error: %s\n", desc);
    exit(-1);
}

void handleGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    std::cout << "GL error: type=" << gl_error_to_string(type) << " id=" << id << " severity =" << severity << " message=" << message << std::endl;
    exit(-1);
}