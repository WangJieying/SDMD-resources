#ifndef __TEXTURE_H
#define __TEXTURE_H

#include <string>
#include <GL/glew.h>
#include <GL/gl.h>

using namespace std;

class Texture2D {
    GLsizei width, height;
    GLint internalFormat;
    GLenum format;
    GLenum type;

public:
    GLuint tex;

    Texture2D (int Width, int Height, GLint intFormat, GLenum Format, GLenum Type);
    ~Texture2D();

    void bind();
    void *getData();
    void setData(void* data);
    void setWrapping(GLint wrap_s, GLint wrap_t);
    void setFiltering(GLint min, GLint mag);
    unsigned char* get_red_texture_data();

    void saveAsPNG(string filename);

private:
    unsigned int formatAsLodePNGtype();
    unsigned int bytesPerPixel();
};

inline void Texture2D::setWrapping(GLint wrap_s, GLint wrap_t) {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
}

inline void Texture2D::setFiltering(GLint min, GLint mag) {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
}

#endif
