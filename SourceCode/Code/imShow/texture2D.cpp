#include <cstdlib> /* Defines NULL */

#include "include/texture2D.hpp"
#include "lodepng/lodepng.h"
#include "include/messages.h"

/* Public methods */
Texture2D::Texture2D(int Width, int Height, GLint intFormat, GLenum Format, GLenum Type) {
    width = Width;
    height = Height;
    internalFormat = intFormat;
    format = Format;
    type = Type;

    glGenTextures(1, &tex);
    bind();
    setData(NULL);
    setWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    setFiltering(GL_LINEAR, GL_LINEAR);

}

Texture2D::~Texture2D() {
    glDeleteTextures(1, &tex);
}

void Texture2D::bind() {
    glBindTexture(GL_TEXTURE_2D, tex);
}

void Texture2D::setData(void *data) {
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                 format, type, data);
}

/* FIXME: Is this flip really nescessary? */
/* Maybe refactor buffer creation etc. to a separate function that is called
 * once before saving the texture, since that is way faster than creating a
 * new buffer every time we want to save a texture.
 */
void Texture2D::saveAsPNG(string filename) {
    unsigned char *data;
    unsigned char *data_flip;

    int i, j, j_flip, idx, idx_flip;
    int channels, c;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    channels = bytesPerPixel();
    data = (unsigned char*) calloc(channels * width * height, sizeof(unsigned char));
    data_flip = (unsigned char*) calloc(channels * width * height,  sizeof(unsigned char));

    bind();
    //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, data);
    vector<unsigned char> image;
    /* Flip the buffer */
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            j_flip = height - j - 1;
            idx = (j * width + i);
            idx_flip = (j_flip * width + i);
            for (c = 0; c < channels; c++) {
                data_flip[channels * idx + c] = data[channels * idx_flip + c];
            }

        }
    }
    // unsigned lodepng_encode24_file(const char* filename,
    //                                const unsigned char* image, unsigned w, unsigned h);

    lodepng_encode24_file(filename.c_str(), data_flip, width, height);

    PRINT(MSG_NORMAL, "Saved texture to file %s\n", filename.c_str());

    free(data);
    free(data_flip);
}

/* Private methods */
unsigned int Texture2D::formatAsLodePNGtype() {
    switch (format) {
    case GL_RGB:
        PRINT(MSG_VERBOSE, "Format is RGB\n");
        return 2;
        break;
    case GL_RGBA:
        PRINT(MSG_VERBOSE, "Format is RGBA\n");
        return 6;
        break;
    default:
        return -1;
        break;
    }
}

unsigned int Texture2D::bytesPerPixel() {
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
