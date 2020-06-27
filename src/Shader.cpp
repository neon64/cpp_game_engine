#include "Shader.h"
#include <glad/glad.h>
#include <iostream>

Shader::Shader(GLuint id, ShaderType type, std::string description) : id(id), type(type), description(description) { }

Shader::~Shader() {
    if(id == 0) { return; }
    std::cout << "glDeleteShader(" << id << ")" << std::endl;
    glDeleteShader(id);
}

void Shader::compile(const std::string &source) {
    std::cout << "compiling shader id=" << id << std::endl;
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

Shader Shader::build(ShaderType type, std::string description, const std::string &source) {
    int id = glCreateShader(type);

    Shader s(id, type, description);
    s.compile(source);

    return std::move(s);
}

GLuint Shader::getId() const {
    return id;
}

Shader::Shader(Shader &&other) : id(std::move(other.id)), type(std::move(other.type)), description(std::move(other.description)) {
    id = 0;
}
