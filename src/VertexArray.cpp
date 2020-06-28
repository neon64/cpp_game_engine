#include "VertexArray.h"

using namespace std;

VertexArray::VertexArray(GLuint id) : OpenGLResource(id) { }

shared_ptr<VertexArray> VertexArray::build() {
    GLuint id;
    glGenVertexArrays(1, &id);
    return make_shared<VertexArray>(id);
}

void VertexArray::destroyResource(){
    glDeleteVertexArrays(1, &id);
}
