
#ifndef GAME_ENGINE_TEXTURING_H
#define GAME_ENGINE_TEXTURING_H

#include "buffer.h"
#include <glad/glad.h>
#include "OpenGLResource.h"
#include "../util.h"

enum class TextureType {
    TEXTURE_1D = GL_TEXTURE_1D,
    TEXTURE_2D = GL_TEXTURE_2D,
    TEXTURE_3D = GL_TEXTURE_3D,
    CUBE_MAP = GL_TEXTURE_CUBE_MAP
};

enum class TransferFormat {
    R8G8B8_UINT,
    R8G8B8A8_UINT
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

struct SamplerCreateInfo {
    SamplerFilter magFilter;
    SamplerFilter minFilter;
    SamplerMipmapMode mipmapMode;
    SamplerAddressMode addressModeU = SamplerAddressMode::REPEAT; // S
    SamplerAddressMode addressModeV = SamplerAddressMode::REPEAT; // T
    SamplerAddressMode addressModeW = SamplerAddressMode::REPEAT; // R in OpenGL
    bool cubemapSeamless = false;
    optional<float> anisotropicFiltering = nullopt;

    static SamplerCreateInfo ALL_LINEAR;
    static SamplerCreateInfo ALL_NEAREST;

    SamplerCreateInfo(SamplerFilter magFilter, SamplerFilter minFilter, SamplerMipmapMode mode);
    SamplerCreateInfo(SamplerFilter magAndMinFilter, SamplerMipmapMode mode);
    SamplerCreateInfo withAddressMode(SamplerAddressMode mode);
    SamplerCreateInfo withCubeMapSeamless(bool seamless);
    SamplerCreateInfo withAnisotropicFiltering(float amount);
};

class Sampler : public OpenGLResource<Sampler> {
public:
    Sampler(GLuint id);

    void destroyResource();

    static Sampler build(SamplerCreateInfo info);

};

template<typename T>
struct TextureBindingInner {

};

template<typename T>
struct TextureBinding {
    T& texture;
    const Sampler& sampler;
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

class Texture2d : public Texture {
public:
    Texture2d(GLuint id, DataFormat format, Dimensions2d size, bool hasMipMaps);

    TextureBinding<Texture2d> withSampler(const Sampler& sampler) {
        return { .texture = *this, .sampler = sampler };
    }
};

#endif //GAME_ENGINE_TEXTURING_H
