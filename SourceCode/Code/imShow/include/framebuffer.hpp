#ifndef __FRAMEBUFFER_H
#define __FRAMEBUFFER_H

#include "texture2D.hpp"
#include "texture3D.hpp"

class Framebuffer {
    GLuint  buffer,
            depthbuffer;
    int     width;
    int     height;
    //int     depth;
    bool    hasDepth;
    bool    is3D;

public:
    Texture2D *texture;
    Texture3D *texture3D;

    Framebuffer(int width, int height, GLint intFormat = GL_RGB,
                GLenum format = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, bool hasDepth = false);
    Framebuffer(Texture2D *tex);
    Framebuffer(Texture3D *tex);
    void bind();
    void unbind();
private:
    void init();
};

#endif
