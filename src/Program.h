#ifndef GAME_ENGINE_PROGRAM_H
#define GAME_ENGINE_PROGRAM_H

#include <initializer_list>
#include "Shader.h"

typedef int ProgramRef;

class Program {
private:


    void attachShader(const Shader& s);
    void detachShader(const Shader& s);

    Program(GLuint id);

    void linkAndValidate();
public:
    const GLuint id;

    Program(const Shader&) = delete;
    Program& operator=(const Shader&) = delete;

    ~Program();

    static Program build(std::initializer_list<Shader> shader);
};


#endif //GAME_ENGINE_PROGRAM_H
