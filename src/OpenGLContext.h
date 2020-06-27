#ifndef GAME_ENGINE_OPENGLCONTEXT_H
#define GAME_ENGINE_OPENGLCONTEXT_H

#include "Window.h"
#include "commands.h"

class OpenGLContext {
public:
    OpenGLContext(Window &window);

    // can only move, not copyable
    OpenGLContext(const OpenGLContext&) = delete;
    OpenGLContext& operator=(const OpenGLContext&) = delete;
    OpenGLContext(OpenGLContext&& other);

    void submit(ClearCommand command);
};


#endif //GAME_ENGINE_OPENGLCONTEXT_H
