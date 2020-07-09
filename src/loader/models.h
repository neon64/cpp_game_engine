
#pragma once

#include "../buffer.h"
#include "../OpenGLContext.h"

struct ModelInfo {
    UntypedBuffer vertices;
    IndexBufferRef indices;
};

optional<ModelInfo> import_model(const std::string &filepath, OpenGLContext &context);