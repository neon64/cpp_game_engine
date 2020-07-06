
#ifndef GAME_ENGINE_TEXTURING_H
#define GAME_ENGINE_TEXTURING_H

#include <glad/glad.h>
#include "OpenGLResource.h"
#include "pipeline.h"
#include "util.h"

enum class TextureType {
    TEXTURE_1D = GL_TEXTURE_1D,
    TEXTURE_2D = GL_TEXTURE_2D,
    TEXTURE_3D = GL_TEXTURE_3D,
    CUBE_MAP = GL_TEXTURE_CUBE_MAP
};

enum class TransferFormat {
    R8G8B8_UINT
};

enum class SamplerFilter {
    NEAREST,
    LINEAR
};

enum class SamplerMipmapMode {
    DISABLED,
    NEAREST,
    LINEAR
};

enum class SamplerAddressMode {
    REPEAT = GL_REPEAT,
    MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
    CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
    CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER
};

class Texture : public OpenGLResource<Texture> {

public:
    Texture(GLuint id, TextureType type, DataFormat format, Dimensions2d size, bool hasMipMaps);

    const bool hasMipMaps;
    const TextureType type;
    const DataFormat format;
    const Dimensions2d size;

    void destroyResource();

};

struct SamplerCreateInfo {
    SamplerFilter magFilter;
    SamplerFilter minFilter;
    SamplerMipmapMode mipmapMode;
    SamplerAddressMode addressModeU; // S
    SamplerAddressMode addressModeV; // T
    SamplerAddressMode addressModeW; // R in OpenGL
    bool cubemapSeamless;
};

class Sampler : public OpenGLResource<Sampler> {
public:
    Sampler(GLuint id);

    void destroyResource();

    static Sampler build(SamplerCreateInfo info);

};

#endif //GAME_ENGINE_TEXTURING_H
