
#include "texturing.h"
#include <cassert>

Texture::Texture(GLuint id, TextureType type, DataFormat format, Dimensions2d size, bool hasMipMaps) : OpenGLResource(id), type(type), format(format), size(size), hasMipMaps(hasMipMaps) {}

void Texture::destroyResource() {
    glDeleteTextures(1, &id);
}

GLuint getMinFilterEnum(SamplerFilter minFilter, SamplerMipmapMode mipmapMode) {
    if(minFilter == SamplerFilter::NEAREST) {
        if(mipmapMode == SamplerMipmapMode::NEAREST) {
            return GL_NEAREST_MIPMAP_NEAREST;
        } else if(mipmapMode == SamplerMipmapMode::LINEAR) {
            return GL_NEAREST_MIPMAP_LINEAR;
        } else if(mipmapMode == SamplerMipmapMode::DISABLED) {
            return GL_NEAREST;
        }
    } else if(minFilter == SamplerFilter::LINEAR) {
        if(mipmapMode == SamplerMipmapMode::NEAREST) {
            return GL_LINEAR_MIPMAP_NEAREST;
        } else if(mipmapMode == SamplerMipmapMode::LINEAR) {
            return GL_LINEAR_MIPMAP_LINEAR;
        } else if(mipmapMode == SamplerMipmapMode::DISABLED) {
            return GL_LINEAR;
        }
    }
    assert(false);
    return 0;
}

Sampler Sampler::build(SamplerCreateInfo info) {
    GLuint id;
    glGenSamplers(1, &id);

    if(info.magFilter == SamplerFilter::NEAREST) {
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else if(info.magFilter == SamplerFilter::LINEAR) {
        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        assert(false);
    }

    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, getMinFilterEnum(info.minFilter, info.mipmapMode));

    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, static_cast<GLint>(info.addressModeU));
    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, static_cast<GLint>(info.addressModeV));
    glSamplerParameteri(id, GL_TEXTURE_WRAP_R, static_cast<GLint>(info.addressModeW));

    glSamplerParameteri(id, GL_TEXTURE_CUBE_MAP_SEAMLESS, info.cubemapSeamless ? GL_TRUE : GL_FALSE);

    return Sampler(id);
}

Sampler::Sampler(GLuint id) : OpenGLResource(id) {}

void Sampler::destroyResource() {
    glDeleteSamplers(1, &id);
}
