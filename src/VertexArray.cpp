#include "VertexArray.h"

using namespace std;

VertexArray::VertexArray(GLuint id) : OpenGLResource(id) { }

VertexArray VertexArray::build() {
    GLuint id;
    glGenVertexArrays(1, &id);
    return VertexArray(id);
}

void VertexArray::destroyResource(){
    glDeleteVertexArrays(1, &id);
}
