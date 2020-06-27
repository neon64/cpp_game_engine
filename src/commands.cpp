#include "commands.h"

#include <optional>

using namespace std;

ClearCommand ClearCommand::clearColor(ColorRGBA color) {
    return { make_optional(color), nullopt, nullopt };
}

ClearCommand ClearCommand::clearColorAndDepth(ColorRGBA color, double depth) {
    return { make_optional(color), make_optional(depth), nullopt };
}
