#ifndef GAME_ENGINE_SHADER_H
#define GAME_ENGINE_SHADER_H

#include <glad/glad.h>
#include <string>

typedef GLuint ShaderType;

/**
 * Corresponds to an OpenGL shader object.
 */
class Shader {
private:
    GLuint id;
    const ShaderType type;
    const std::string description;

    Shader(GLuint id, ShaderType type, std::string description);

    void compile(const std::string &source);

public:
    GLuint getId() const;

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other);

    ~Shader();

    static Shader build(ShaderType type, std::string description, const std::string& source);
};


#endif //GAME_ENGINE_SHADER_H
