#ifndef GAME_ENGINE_PROGRAM_H
#define GAME_ENGINE_PROGRAM_H

#include <initializer_list>
#include "Shader.h"
#include "OpenGLResource.h"

class Program : public OpenGLResource<Program> {
public:
    void attachShader(const Shader& s);
    void detachShader(const Shader& s);

    Program(GLuint id);

    void linkAndValidate();

    void destroyResource();
};


#endif //GAME_ENGINE_PROGRAM_H
