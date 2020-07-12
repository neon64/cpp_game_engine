
#pragma once

#include <memory>
#include "cache.h"
#include "../graphics/Shader.h"
#include "../graphics/OpenGLContext.h"

class ShaderCache : public ResourceCache<string, Shader> {
};