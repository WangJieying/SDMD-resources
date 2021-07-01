#ifndef __GLSLSHADER_H
#define __GLSLSHADER_H

#include <string>

using namespace std;

class GLSLShader
{
        GLenum type;

        string file;
        char *src;

    public:
        GLuint shader;

        GLSLShader(string srcPath, GLenum type);
        ~GLSLShader();

        void compile();
        void reload();

    private:
        void loadSourceFromFile();
        string typeAsString();
};

#endif