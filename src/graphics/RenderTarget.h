
#ifndef GAME_ENGINE_RENDERTARGET_H
#define GAME_ENGINE_RENDERTARGET_H

#include "OpenGLResource.h"
#include "texturing.h"
#include "../Window.h"
#include <memory>
#include <variant>

using namespace std;

class DefaultRenderTarget {
    Window& window;
public:
    DefaultRenderTarget(Window& window);
    Dimensions2d getSize() const;
    GLuint getId() const;
};

//struct FramebufferAttachment {
//    variant<ColorAttachmentType, DepthAttachmentType, StencilAttachmentType> type;
//    unique_ptr<Attachment> contents;
//
//    GLuint getType() const;
//};

class ColorAttachment {
public:
    virtual void attach(uint32_t index) = 0;

    virtual Dimensions2d getSize() const = 0;
};

class DepthAttachment {
public:
    virtual void attach() = 0;
    virtual Dimensions2d getSize() const = 0;
};

class StencilAttachment {
public:
    virtual void attach() = 0;
    virtual Dimensions2d getSize() const = 0;
};

using ColorAttachments = unordered_map<int, unique_ptr<ColorAttachment>>;
using MaybeDepthAttachment = optional<unique_ptr<DepthAttachment>>;
using MaybeStencilAttachment = optional<unique_ptr<StencilAttachment>>;


enum class RenderbufferInternalFormat {
    D16_UNORM = GL_DEPTH_COMPONENT16,
    D24_UNORM = GL_DEPTH_COMPONENT24,
    D32_SFLOAT = GL_DEPTH_COMPONENT32F
};

class Renderbuffer : public OpenGLResource<Renderbuffer> {
    RenderbufferInternalFormat format;
    Dimensions2d size;
public:
    Dimensions2d getSize() const;

public:
    Renderbuffer(GLuint id, RenderbufferInternalFormat format, Dimensions2d size);

    void destroyResource();
};

class OwnedDepthRenderbufferAttachment : public DepthAttachment {
    Renderbuffer renderbuffer;
public:
    OwnedDepthRenderbufferAttachment(Renderbuffer &&renderbuffer);
    virtual void attach();
    virtual Dimensions2d getSize() const;
};

template<typename T>
class OwnedColorTextureAttachment : public ColorAttachment {
    T texture;
    uint32_t mipmapLevel;
public:
    T& getTexture() {
        return texture;
    }

    OwnedColorTextureAttachment(T&& texture, uint32_t mipmapLevel) : texture(
            std::forward<T>(texture)), mipmapLevel(mipmapLevel) {}

    TextureBinding<T> sample(Sampler& sampler) {
        return getTexture().withSampler(sampler);
    }

    virtual void attach(uint32_t index);
    virtual Dimensions2d getSize() const {
        return texture.size;
    }
};

template<typename T>
class OwnedDepthTextureAttachment : public DepthAttachment {
    T texture;
    uint32_t mipmapLevel;

public:
    OwnedDepthTextureAttachment(T&& texture, uint32_t mipmapLevel) : texture(std::forward<T>(texture)), mipmapLevel(mipmapLevel) {}

    T& getTexture() {
        return texture;
    }

    TextureBinding<T> sample(Sampler& sampler) {
        return getTexture().withSampler(sampler);
    }

    virtual void attach();
    virtual Dimensions2d getSize() const {
        return texture.size;
    }
};

class Framebuffer : public OpenGLResource<Framebuffer> {
    Dimensions2d size;
    ColorAttachments colorAttachments;
    MaybeDepthAttachment depthAttachment;
    MaybeStencilAttachment stencilAttachment;
public:
//    Framebuffer(Framebuffer&& other) : OpenGLResource(std::move(other.id)), size(other.size), colorAttachments(std::move(other.colorAttachments)), depthAttachment(std::move(other.depthAttachment)), stencilAttachment(std::move(other.stencilAttachment)) {
//        other.id = 0;
//    }
    Framebuffer(GLuint id, Dimensions2d size, ColorAttachments colorAttachments, MaybeDepthAttachment depthAttachment, MaybeStencilAttachment stencilAttachment);

    Dimensions2d getSize() const;

    Rect2d getRect() const;

    optional<DepthAttachment*> getDepthAttachment();
    optional<ColorAttachment*> getColorAttachment(uint32_t index);

    void destroyResource();
};


#endif //GAME_ENGINE_RENDERTARGET_H
