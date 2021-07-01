#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "include/messages.h"
#include "include/io.hpp"
#include "include/glslShader.h"

using namespace std;

/* Public Functions */
GLSLShader::GLSLShader(string srcPath, GLenum shader_type) {
    file = srcPath;
    type = shader_type;

    loadSourceFromFile();
}

GLSLShader::~GLSLShader() {
    free(src);
    glDeleteShader(shader);
}

void GLSLShader::compile() {
    GLint param, length;
    GLchar info [1000];
    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar **) &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
    if (param == GL_FALSE) {
        PRINT(MSG_ERROR, "Compiling shader failed\n");
        glGetShaderInfoLog(shader, 1000, &length, info);
        PRINT(MSG_ERROR, "%s\n", info);
        exit(1);
    }

    PRINT(MSG_NORMAL, "Compiled %s shader '%s' succesfully\n",
            typeAsString().c_str(), file.c_str());
}

/* Private Functions */
void GLSLShader::loadSourceFromFile() {

    if(!readFile(file.c_str(), (char **) &this->src)){
        PRINT(MSG_ERROR, "Cannot load shader from file `%s'\n", file.c_str());
        exit(-1);
    }
    PRINT(MSG_VERBOSE,"(%s) Shader content: \n%s\n", typeAsString().c_str(), this->src);
    
}

string GLSLShader::typeAsString() {
    string str;

    switch (type) {
        case GL_VERTEX_SHADER:
            str = "vertex";
            break;
        case GL_GEOMETRY_SHADER_EXT:
            str = "geometry";
            break;
        case GL_FRAGMENT_SHADER:
            str = "fragment";
            break;
        default:
            str = "<unknown>";
            break;
    }

    return str;
}
