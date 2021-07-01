#include <cstdlib> /* Defines NULL */

#include "include/texture3D.hpp"
#include "include/lodepng.h"
#include "include/messages.h"

/* Public methods */
Texture3D::Texture3D(int Width, int Height, int Depth, GLint intFormat, GLenum Format, GLenum Type) {
    width = Width;
    height = Height;
    depth = Depth;
    internalFormat = intFormat;
    format = Format;
    type = Type;

    glGenTextures(1, &tex);
    bind();
    setData(NULL);
    setWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    setFiltering(GL_LINEAR, GL_LINEAR);
}

Texture3D::~Texture3D() {
    glDeleteTextures(1, &tex);
}

void Texture3D::bind() {
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
}

void Texture3D::setData(void *data) {
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, width, height, depth);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, depth, internalFormat, format, data);
    // glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, width, height, depth, 0,
    //              format, type, data);
}

/* FIXME: Is this flip really nescessary? */
/* Maybe refactor buffer creation etc. to a separate function that is called
 * once before saving the texture, since that is way faster than creating a
 * new buffer every time we want to save a texture.
 */
void Texture3D::saveAsPNG(string filename) {
    PRINT(MSG_ERROR, "Cannot save 3D texture as PNG.\n");
}

/* Private methods */
unsigned int Texture3D::formatAsLodePNGtype() {
    return 0;
}

unsigned int Texture3D::bytesPerPixel() {
    switch (format) {
    case GL_RGB:
        return 3;
        break;
    case GL_RGBA:
        return 4;
        break;
    default:
        return 0;
        break;
    }
}
