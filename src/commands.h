#ifndef GAME_ENGINE_COMMANDS_H
#define GAME_ENGINE_COMMANDS_H

#include <optional>
#include <glad/glad.h>

#include "ColorRGBA.h"

struct ClearCommand {
    const std::optional<ColorRGBA> color;
    const std::optional<GLdouble> depth;
    const std::optional<GLint> stencil;

    static ClearCommand clearColor(ColorRGBA color);
    static ClearCommand clearColorAndDepth(ColorRGBA color, double depth);
};


#endif //GAME_ENGINE_COMMANDS_H
