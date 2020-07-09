#include "Program.h"
#include <glad/glad.h>
#include <iostream>
#include <cstdarg>

void Program::attachShader(const Shader& shader) {
    glAttachShader(id, shader.getId());
}

void Program::detachShader(const Shader& shader) {
    glDetachShader(id, shader.getId());
}

void Program::linkAndValidate() {
    glLinkProgram(id);
    glValidateProgram(id);

    int status;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        std::cerr << "program linking failed (id=" << getId() << ")" << std::endl;
        int maxLength;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &maxLength);

        char *infoLog = (char *) malloc(sizeof(char) * maxLength);
        int bytesWritten;
        glGetProgramInfoLog(id, maxLength, &bytesWritten, infoLog);

        if(bytesWritten == 0) {
            std::cerr << "no info log" << endl;
        } else {
            std::cerr << "info log: " << infoLog << "(bytes=" << bytesWritten << ")" << std::endl;
        }

        free(infoLog);
    }
}

//Program Program::build(std::initializer_list<Shader*> shaders) {
//    GLuint programId = glCreateProgram();
//    Program program(programId);
//
//    for(auto &shader : shaders) {
//        program.attachShader(shader);
//    }
//
//    program.linkAndValidate();
//
//    return program;
//}

void Program::destroyResource() {
    glDeleteProgram(id);
}

Program::Program(GLuint id) : OpenGLResource(id) {

}
