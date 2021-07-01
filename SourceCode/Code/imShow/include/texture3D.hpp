#ifndef __TEXTURE3D_H
#define __TEXTURE3D_H

#include <string>
#include <GL/glew.h>
#include <GL/gl.h>

using namespace std;

class Texture3D {
    GLsizei width, height, depth;
    GLint internalFormat;
    GLenum format;
    GLenum type;

public:
    GLuint tex;

    Texture3D(int Width, int Height, int Depth, GLint intFormat, GLenum Format, GLenum Type);
    ~Texture3D();

    void bind();
    void *getData();
    void setData(void* data);
    void setWrapping(GLint wrap_s, GLint wrap_t);
    void setFiltering(GLint min, GLint mag);

    void saveAsPNG(string filename);

private:
    unsigned int formatAsLodePNGtype();
    unsigned int bytesPerPixel();
};

inline void Texture3D::setWrapping(GLint wrap_s, GLint wrap_t) {
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap_t);
}

inline void Texture3D::setFiltering(GLint min, GLint mag) {
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, mag);
}


#endif
