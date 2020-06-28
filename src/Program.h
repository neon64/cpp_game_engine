#ifndef GAME_ENGINE_PROGRAM_H
#define GAME_ENGINE_PROGRAM_H

#include <initializer_list>
#include "Shader.h"
#include "OpenGLResource.h"

class Program : public OpenGLResource<Program> {
private:
    void attachShader(const Shader& s);
    void detachShader(const Shader& s);

    Program(GLuint id);

    void linkAndValidate();
public:
    void destroyResource();

    static Program build(std::initializer_list<Shader> shader);
};


#endif //GAME_ENGINE_PROGRAM_H
