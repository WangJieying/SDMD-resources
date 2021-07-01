#include <stdlib.h>

#include "include/framebuffer.hpp"
#include "include/messages.h"

/* Public methods */
Framebuffer::Framebuffer(int width, int height, GLint intFormat, GLenum format, GLenum type, bool hasDepth) {
    texture = new Texture2D(width, height, intFormat, format, type);

    this->width = width;
    this->height = height;
    this->hasDepth = hasDepth;
    is3D = false;
    init();
}

Framebuffer::Framebuffer(Texture2D *tex) {
    texture = tex;
    is3D = false;
    hasDepth = true;
    init();
}

Framebuffer::Framebuffer(Texture3D *tex) {
    texture3D = tex;
    is3D = true;
    hasDepth = false;
    init();

}

void Framebuffer::bind() {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buffer);

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, this->width, this->height);
}

void Framebuffer::unbind() {
    glPopAttrib();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

/* Private methods */
void Framebuffer::init() {
    GLenum status;

    glGenFramebuffersEXT(1, &buffer);
    bind();
    /* Attach depth buffer if necessary */
    if (hasDepth) {
        glGenRenderbuffersEXT(1, &depthbuffer);
        PRINT(MSG_VERBOSE, "Depth Buffer ID: %u\n", depthbuffer);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);
        PRINT(MSG_VERBOSE, "Using depth buffer!\n");
    }

    if (is3D) {
        glFramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                  GL_TEXTURE_3D, texture3D->tex, 0, 0);
    } else {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                  GL_TEXTURE_2D, texture->tex, 0);
    }


    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
        PRINT(MSG_ERROR, "Framebuffer status incomplete! (status = %d)\n", status);
        exit(1);
    }

    unbind();
}
