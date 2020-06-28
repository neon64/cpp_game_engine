#include "commands.h"

#include <optional>

using namespace std;

ClearCommand::ClearCommand(ColorRGBA color)
    : color(make_optional(color)), depth(nullopt), stencil(nullopt) { }

ClearCommand::ClearCommand(ColorRGBA color, double depth)
    : color(make_optional(color)), depth(make_optional(depth)), stencil(nullopt){ }
