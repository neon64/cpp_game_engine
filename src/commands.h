#ifndef GAME_ENGINE_COMMANDS_H
#define GAME_ENGINE_COMMANDS_H

#include <optional>
#include <glad/glad.h>
#include <memory>

#include "ColorRGBA.h"
#include "pipeline.h"

using namespace std;

struct ClearCommand {
    const std::optional<ColorRGBA> color;
    const std::optional<GLdouble> depth;
    const std::optional<GLint> stencil;

    ClearCommand(ColorRGBA color);
    ClearCommand(ColorRGBA color, double depth);
};

struct DrawCommand {
    const shared_ptr<GraphicsPipeline> pipeline;
};


#endif //GAME_ENGINE_COMMANDS_H
