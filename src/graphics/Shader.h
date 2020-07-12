#ifndef GAME_ENGINE_SHADER_H
#define GAME_ENGINE_SHADER_H

#include <glad/glad.h>
#include <string>
#include <memory>

#include "OpenGLResource.h"

using namespace std;

class OpenGLContext;

enum ShaderType {
    VERTEX = GL_VERTEX_SHADER,
    FRAGMENT = GL_FRAGMENT_SHADER
};

/**
 * Corresponds to an OpenGL shader object.
 */
class Shader : public OpenGLResource<Shader> {
private:
    friend class OpenGLContext;

    const ShaderType type;
    const std::string description;

    void compile(const std::string_view &source);
public:
    Shader(GLuint id, ShaderType type, std::string description);

    void destroyResource();
};


#endif //GAME_ENGINE_SHADER_H
