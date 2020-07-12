#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <memory>

using namespace std;

Shader::Shader(GLuint id, ShaderType type, std::string description) : OpenGLResource(id), type(type), description(description) { }

void Shader::destroyResource() {
    std::cout << "glDeleteShader(" << getId() << ")" << std::endl;
    glDeleteShader(id);
}

void Shader::compile(const std::string_view &source) {
    std::cout << "compiling shader '" << description << "' (id=" << getId() << ")" << std::endl;
    int len = source.size();
    const char *first_string = source.data();
    glShaderSource(id, 1, &first_string, &len);
    glCompileShader(id);

    int status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        int maxLength;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

        char *infoLog = (char *) malloc(sizeof(char) * maxLength);
        int bytesWritten;
        glGetShaderInfoLog(id, maxLength, &bytesWritten, infoLog);

        std::cerr << infoLog << std::endl;

        free(infoLog);
    }
}