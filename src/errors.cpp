
#include <glad/glad.h>
#include <cassert>
#include <iostream>

using namespace std;

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
            printf("0x%x\n", err);
            return "unknown";
    }
}

const char *debug_message_type_to_string(GLenum type) {
    switch(type) {
        case GL_DEBUG_TYPE_ERROR:                      return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:                  return "DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:                 return "UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:             return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:                return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:               return "MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP:                 return "PUSH_GROUP";
        case GL_DEBUG_TYPE_POP_GROUP:             return "POP_GROUP";
        case GL_DEBUG_TYPE_OTHER:             return "OTHER";
        default:
            assert(false);
            return NULL;
    }
}

const char *debug_message_severity_to_string(GLenum severity) {
    switch(severity) {
        case GL_DEBUG_SEVERITY_HIGH:                      return "HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM:                  return "MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:                 return "LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION:             return "NOTIFICATION";
        default:
            assert(false);
            return NULL;
    }
}

void handleGLFWError(int code, const char* desc) {
    cerr << "GLFW error: " << desc << endl;
}

void handleGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    cerr << "GL error: type=" << debug_message_type_to_string(type) << ", id=" << id << ", severity=" << debug_message_severity_to_string(severity) << ", message={" << message << "}" << endl;
}