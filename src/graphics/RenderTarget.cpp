
#include "RenderTarget.h"
#include "texturing.h"
#include <cassert>

Framebuffer::Framebuffer(GLuint id, Dimensions2d size,
                         ColorAttachments colorAttachments,
                         MaybeDepthAttachment depthAttachment,
                         MaybeStencilAttachment stencilAttachment) : OpenGLResource(id), size(size),
                                                                     colorAttachments(
                                                                             std::move(colorAttachments)),
                                                                     depthAttachment(std::move(depthAttachment)),
                                                                     stencilAttachment(std::move(
                                                                             stencilAttachment)) {

}

void Framebuffer::destroyResource() {
    glDeleteFramebuffers(1, &id);
}

Dimensions2d Framebuffer::getSize() const {
    return size;
}

Rect2d Framebuffer::getRect() const {
    return Rect2d(Point2d(0, 0), getSize());
}

optional<DepthAttachment*> Framebuffer::getDepthAttachment() {
    if(!depthAttachment.has_value()) {
        return nullopt;
    }
    return make_optional(&*depthAttachment.value());
}

optional<ColorAttachment*> Framebuffer::getColorAttachment(uint32_t index) {
    auto it = colorAttachments.find(index);
    if(it == colorAttachments.end()) {
        return nullopt;
    }
    return make_optional(&*it->second);
}

Renderbuffer::Renderbuffer(GLuint id, RenderbufferInternalFormat format, Dimensions2d size) : OpenGLResource(id),
                                                                                              format(format),
                                                                                              size(size) {}

void Renderbuffer::destroyResource() {
    glDeleteRenderbuffers(1, &id);
}

Dimensions2d Renderbuffer::getSize() const {
    return size;
}

OwnedDepthRenderbufferAttachment::OwnedDepthRenderbufferAttachment(Renderbuffer &&renderbuffer) : renderbuffer(
        std::move(renderbuffer)) {}

void OwnedDepthRenderbufferAttachment::attach() {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.getId());
}

Dimensions2d OwnedDepthRenderbufferAttachment::getSize() const {
    return renderbuffer.getSize();
}

Dimensions2d DefaultRenderTarget::getSize() const {
    return window.getSize();
}

GLuint DefaultRenderTarget::getId() const {
    // OpenGL default framebuffer
    return 0;
}

DefaultRenderTarget::DefaultRenderTarget(Window &window) : window(window) {

}

template<>
void OwnedColorTextureAttachment<Texture2d>::attach(uint32_t index) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, static_cast<GLuint>(texture.type),
            texture.getId(), mipmapLevel);
}


template<>
void OwnedDepthTextureAttachment<Texture2d>::attach() {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, static_cast<GLuint>(texture.type),
            texture.getId(), mipmapLevel);
}
