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

void Shader::compile(const std::string &source) {
    std::cout << "compiling shader id=" << getId() << std::endl;
    int len = source.size();
    const char *first_string = source.c_str();
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

        std::cout << infoLog << std::endl;

        free(infoLog);
    }
}

shared_ptr<Shader> Shader::build(ShaderType type, std::string description, const std::string &source) {
    GLuint id = glCreateShader(type);

    auto s = make_shared<Shader>(id, type, description);
    s->compile(source);

    return s;
}